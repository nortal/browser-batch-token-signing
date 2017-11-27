/*
* Estonian ID card plugin for web browsers
*
* Copyright (C) 2010-2011 Codeborne <info@codeborne.com>
*
* This is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
*/

#include "stdafx.h"
#include "EstEIDIEPluginBHO.h"

#include "BinaryUtils.h"
#include "CertificateSelector.h"
#include "esteid_error.h"
#include "HostExceptions.h"
#include "Labels.h"
#include "Logger.h"
#include "Signer.h"
#include "version.h"
#include "PinDialogCNG.h"
#include "ProgressBar.h"

#include <memory>
#include <string.h>


using namespace std;

bool cancelSigning;                  // can be modified in progress bar dialog
CProgressBarDialog* progressBarDlg;  // may need to be accesses in request handler
static time_t startTime;             // for performance logging

int getHashCount(const char* allHashes) {
  // calculate the number of hashes in the given hash string
  int len = 0;
  int count = 0;
  if (allHashes) {
    while (allHashes[len]) {
      if (allHashes[len] == ',') {
        count++;
      }
      len++;
    }
    count++;
  }
  return count;
}

void processMessages() {
  //int cnt = 0;
  MSG msg;
  while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))// && cnt++ < 10)
  {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
}

STDMETHODIMP CEstEIDIEPluginBHO::SetSite(IUnknown* pUnkSite) {
	_log("");
	IObjectWithSiteImpl<CEstEIDIEPluginBHO>::SetSite(pUnkSite);

	CComPtr<IServiceProvider> pSP;
	HRESULT hr = GetSite(IID_IServiceProvider, reinterpret_cast<LPVOID *>(&pSP));
	if(S_OK != hr) {
		return hr;
	}
	hr = pSP->QueryService(IID_IWebBrowserApp, IID_IWebBrowser2, reinterpret_cast<LPVOID *>(&webBrowser));

	return S_OK;
}

STDMETHODIMP CEstEIDIEPluginBHO::get_multipleHashesSupported(BSTR *result) {
  _log("");
  *result = _bstr_t("true").Detach();
  return S_OK;
}

STDMETHODIMP CEstEIDIEPluginBHO::get_version(BSTR *result) {
	_log("");
	*result = _bstr_t(ESTEID_PLUGIN_VERSION).Detach();
	return S_OK;
}

STDMETHODIMP CEstEIDIEPluginBHO::get_pluginLanguage(BSTR *result) {
	_log("");
	*result = _bstr_t(language ? language : _T("")).Detach();
	return S_OK;
}


STDMETHODIMP CEstEIDIEPluginBHO::put_pluginLanguage(BSTR _language) {
	USES_CONVERSION;
	_log("");
	language = _bstr_t(_language).Detach();
	Labels::l10n.setLanguage(W2A(language));
	return S_OK;
}


STDMETHODIMP CEstEIDIEPluginBHO::get_errorMessage(BSTR *result) {
	_log("");
	*result = _bstr_t(errorMessage.c_str()).Detach();
	return S_OK;
}

STDMETHODIMP CEstEIDIEPluginBHO::get_errorCode(BSTR *result) {
	_log("");
	*result = _bstr_t(errorCode).Detach();
	return S_OK;
}

void CEstEIDIEPluginBHO::isSiteAllowed() {
	_log("");
	if(!webBrowser) {
		_log("Browser object is not initialized!!!");
		NotAllowedException("Site not allowed");
	}

	CComBSTR url_buffer;
	webBrowser->get_LocationURL(&url_buffer);
	BOOL allowed = wcsstr(url_buffer, _T("https://")) == url_buffer;
#ifdef DEVELOPMENT_MODE
	allowed = TRUE;
	_log("*** Development Mode, all protocols allowed ***");
#endif	
	if(!allowed)
		throw NotAllowedException("Site not allowed");
}

STDMETHODIMP CEstEIDIEPluginBHO::getCertificate(VARIANT filter, IDispatch **certificate) {
	_log("");
	try {
		isSiteAllowed();
		if (cert) {
			// todo: check if card has changed
			cert.Release();
			cert = NULL;
		}
		cert.CoCreateInstance(CLSID_EstEIDCertificate);
		cert->Init(filter);
		CComPtr<IEstEIDCertificate> copy;
		cert.CopyTo(&copy);
		*certificate = copy.Detach();
		clearErrors();
		_log("Get certificate ended");
		return S_OK;
	}
	catch (const BaseException &e) {
		_log("Exception caught when getting certificate: %s", e.what());
		setError(e);
		return Error(errorMessage.c_str());
	}
}

STDMETHODIMP CEstEIDIEPluginBHO::sign(BSTR id, BSTR hashhex, BSTR _language, VARIANT info, BSTR *signature) {
	_log("");
	USES_CONVERSION;
	try {
		std::string signatures(""); // for returned signatures
		int currentHash = 0;
		HWND hWndPB = NULL;

		isSiteAllowed();

		// convert OLE string to std::string for easier handling
		wstring hashToSign(hashhex, SysStringLen(hashhex));
		std::string hashString(hashToSign.begin(), hashToSign.end());

		_log("All hashes: '%s'", hashString.c_str());

		PinDialogCNG::ResetPIN();

		int hashCount = getHashCount(hashString.c_str());
		bool isMassSigning = (hashCount > 1);

		int hashPos = 0; // search position in the complete hash string

		// get the first hash string from the comma separated list
		// hashPos is updated in getNextHash()
		std::string currHash = getNextHash(hashString, hashPos, ",");

		// While we have a hash string, and signing not cancelled by user...
		cancelSigning = false;
		while (currHash != "" && !cancelSigning)
		{
			const std::vector<unsigned char> currHashBin = BinaryUtils::hex2bin(currHash);

			CComBSTR idHex;
			cert->get_id(&idHex);
			if (idHex != id)
				throw NoCertificatesException();

			language = _bstr_t(_language).Detach();
			Labels::l10n.setLanguage(W2A(language));

			_log("Creating signer...");
			CComBSTR certHex;
			cert->get_cert(&certHex);
			unique_ptr<Signer> signer(Signer::createSigner(BinaryUtils::hex2bin(W2A(certHex))));
			if (info.vt == VT_BSTR && !signer->showInfo(W2A_CP(info.bstrVal, CP_UTF8)))
				throw UserCancelledException();

			// ask PIN if needed
			if (isMassSigning && !PinDialogCNG::HasPIN()) {
				_log("Asking PIN...");
				BOOL freeKeyHandle = false;
				NCRYPT_KEY_HANDLE key = signer->getCertificatePrivateKey(currHashBin, &freeKeyHandle);
				int result = PinDialogCNG::AskPIN(key);
				if (freeKeyHandle) NCryptFreeObject(key);
				if (result == SCARD_W_CHV_BLOCKED) {
					PinDialogCNG::ResetPIN();
					throw PinBlockedException();
				}
				else if (result == SCARD_W_CANCELLED_BY_USER) {
					PinDialogCNG::ResetPIN();
					throw UserCancelledException();
				}
				else if (result != ERROR_SUCCESS) {
					PinDialogCNG::ResetPIN();
					throw TechnicalException("Signing failed: PIN/card error.");
				}
			}

			_log("Setting PIN to signer...");
			signer->setPin(PinDialogCNG::GetPIN());

			if (hashCount > 2 && !progressBarDlg) {
				progressBarDlg = new CProgressBarDialog(hashCount);
				hWndPB = progressBarDlg->Create(::GetActiveWindow(), 0);
				progressBarDlg->ShowWindow(SW_SHOWNORMAL);
				SendNotifyMessage(hWndPB, WM_UPDATE_PROGRESS, -1, 0);
			}
			currentHash++;
			if (startTime == 0) {
				time(&startTime);
			}

			_log("Signing...");
			vector<unsigned char> result = signer->sign(currHashBin);
			string resultString = BinaryUtils::bin2hex(result);

			// append the signature to the comma separated signature list
			_log("Appending signature '%s'.", resultString);
			signatures += (signatures.length() ? "," + resultString : resultString);

			// get the next hash string (or "" if nothing is left).
			_log("Getting next hash, hashPos=%d...", hashPos);
			currHash = getNextHash(hashString, hashPos, ",");

			// update progress bar
			if (progressBarDlg && hWndPB && !cancelSigning) {
				SendNotifyMessage(hWndPB, WM_UPDATE_PROGRESS, 0, 0);
			}
			// process pending Windows messages
			processMessages();
		}

		PinDialogCNG::ResetPIN();

		if (cancelSigning) {
			_log("CNG mass signing failed, user canceled while signing hash %d of %d.", currentHash, hashCount);
			throw UserCancelledException("Signing was cancelled");
		}

		_log("%d hashes signed in %d seconds.", currentHash, (int)difftime(time(NULL), startTime));
		if (progressBarDlg) {
			if (progressBarDlg->IsWindow())
				progressBarDlg->DestroyWindow();
			delete progressBarDlg;
			progressBarDlg = NULL;
		}

		_log("All signatures: '%s'", signatures.c_str());

		*signature = _bstr_t(signatures.c_str()).Detach();

		clearErrors();
		_log("Signing ended");
		return S_OK;
	}
	catch (const BaseException &e) {
		_log("Exception caught during signing: %s", e.what());
		setError(e);

		PinDialogCNG::ResetPIN();
		if (progressBarDlg) {
			if (progressBarDlg->IsWindow())
				progressBarDlg->DestroyWindow();
			delete progressBarDlg;
			progressBarDlg = NULL;
		}

		return Error(errorMessage.c_str());
	}
}

void CEstEIDIEPluginBHO::clearErrors() {
	_log("");
	errorCode = 0;
	errorMessage.clear();
}

void CEstEIDIEPluginBHO::setError(const BaseException &exception) {
	errorCode = exception.getErrorCode();
	errorMessage.assign(exception.what());
	_log("Set error: %s (HEX %Xh, DEC %u)", errorMessage.c_str(), exception.getErrorCode(), exception.getErrorCode());
}

std::string CEstEIDIEPluginBHO::getNextHash(std::string allHashes, int& position, char* separator)
{
  std::string result("");
  bool found = false;

  // initialize search
  const char* str = allHashes.c_str();
  str += position;

  // skip separator in the beginning of search
  if (*str == *separator)
  {
    str++;
    position++;
  }

  // store the current position (beginning of substring)
  const char *begin = str;

  // while separator not found and not at end of string..
  while (*str != *separator && *str)
  {
    // ..go forward in the string.
    str++;
    position++;
  }

  // return what we've got, which is either empty string or a hash string
  result = std::string(begin, str);
  return result;
}
