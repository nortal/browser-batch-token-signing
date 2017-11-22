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
#include <string.h>
#include <stdlib.h>
#include "EstEIDIEPluginBHO.h"
#include "esteid_error.h"
#include "version.h"
#include "SignerFactory.h"
#include "HostExceptions.h"
#include "PinDialog.h"
#include "ProgressBar.h"

#define FAIL_IF_SITE_IS_NOT_ALLOWED if(!isSiteAllowed()){return Error((this->errorMessage).c_str());}

extern "C" {
extern int EstEID_errorCode;
}

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
	EstEID_log("");
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
  EstEID_log("");
  *result = _bstr_t("true").Detach();
  return S_OK;
}

STDMETHODIMP CEstEIDIEPluginBHO::get_version(BSTR *result) {
	EstEID_log("");
	*result = _bstr_t(ESTEID_PLUGIN_VERSION).Detach();
	return S_OK;
}

STDMETHODIMP CEstEIDIEPluginBHO::get_pluginLanguage(BSTR *result) {
	EstEID_log("");
	*result = _bstr_t(this->language ? this->language : _T("")).Detach();
	return S_OK;
}


STDMETHODIMP CEstEIDIEPluginBHO::put_pluginLanguage(BSTR language) {
	EstEID_log("");
	this->language = _bstr_t(language).Detach();
	return S_OK;
}


STDMETHODIMP CEstEIDIEPluginBHO::get_errorMessage(BSTR *result) {
	EstEID_log("");
	*result = _bstr_t(this->errorMessage.c_str()).Detach();
	return S_OK;
}

STDMETHODIMP CEstEIDIEPluginBHO::get_errorCode(BSTR *result) {
	EstEID_log("");
	*result = _bstr_t(this->errorCode).Detach();
	return S_OK;
}

BOOL CEstEIDIEPluginBHO::isSiteAllowed() {
	EstEID_log("");
	BSTR url_buffer;
	if(webBrowser == NULL){
		EstEID_log("Browser object is not initialized!!!");
		return FALSE;
	}

	webBrowser->get_LocationURL(&url_buffer);
	BOOL allowed = wcsstr(url_buffer, _T("https://")) == url_buffer;
#ifdef DEVELOPMENT_MODE
	allowed = TRUE;
	EstEID_log("*** Development Mode, all protocols allowed ***");
#endif	
	if(!allowed){
		this->errorCode = ESTEID_SITE_NOT_ALLOWED;
		this->errorMessage.assign("Site not allowed");
		EstEID_log("Protocol not allowed");
	}
	SysFreeString(url_buffer);	
	return allowed;
}

STDMETHODIMP CEstEIDIEPluginBHO::getCertificate(IDispatch **_certificate){
	EstEID_log("");
	FAIL_IF_SITE_IS_NOT_ALLOWED;
	try {
		if(!this->certificate || !isSameCardInReader(this->certificate)) {
			this->certificate.CoCreateInstance(CLSID_EstEIDCertificate);
		}
		CComPtr<IEstEIDCertificate> cert;
		this->certificate.CopyTo(&cert);
		*_certificate = cert.Detach();
		clearErrors();
		return S_OK;
	}
	catch (BaseException &e) {
		EstEID_log("Exception caught when getting certificate: %s: %s", e.getErrorMessage().c_str(), e.getErrorDescription().c_str());
		setError(e);
		return Error((this->errorMessage).c_str());
	}
}

BOOL CEstEIDIEPluginBHO::isSameCardInReader(CComPtr<IEstEIDCertificate> _cert){ //todo: must check is card changed
	/* not implemented yet */
	this->certificate = NULL;
	return false;
}

std::string CEstEIDIEPluginBHO::askPin(int pinLength) {
  if (!pinDialog) {
    pinDialog = new CPinDialog();
  }
  pinDialog->setAttemptsRemaining(3);
  char* signingPin = pinDialog->getPin();
  return std::string(signingPin);
}

STDMETHODIMP CEstEIDIEPluginBHO::sign(BSTR id, BSTR hash, BSTR language, BSTR *signature){
	LOG_LOCATION;
	FAIL_IF_SITE_IS_NOT_ALLOWED;
	USES_CONVERSION;
	try {
    // convert OLE string to std::string for easier handling
	wstring hashToSign(hash, SysStringLen(hash));
	std::string hashString(hashToSign.begin(), hashToSign.end());
    std::string signatures(""); // for returned signatures
    int hashPos = 0;            // search position in the complete hash string
    std::string pin("");
    int currentHash = 0;
    HWND hWndPB = NULL;
	CPinDialogCNG::ResetPIN();

    EstEID_log("All hashes: '%s'", hashString.c_str());

    int hashCount = getHashCount(hashString.c_str());
    bool isMassSigning = (hashCount > 1);

    // get the first hash string from the comma separated list
    // hashPos is updated in getNextHash()
    std::string currHash = getNextHash(hashString, hashPos, ",");

    // While we have a hash string, and signing not cancelled by user...
    cancelSigning = false;
    while (currHash != "" && !cancelSigning)
    {
      char * certId = W2A(id);
      EstEID_log("Creating signer...");
      Signer * signer = SignerFactory::createSigner(currHash, certId);

      // ask PIN if needed
      if (isMassSigning && !CPinDialogCNG::HasPIN()) {
        BOOL freeKeyHandle = false;
        NCRYPT_KEY_HANDLE key = signer->getCertificatePrivateKey(&freeKeyHandle);
        int result = CPinDialogCNG::AskPIN(key);
        if (freeKeyHandle) NCryptFreeObject(key);
        if (result == SCARD_W_CHV_BLOCKED) {
          CPinDialogCNG::ResetPIN();
          throw PinBlockedException();
        }
        else if (result == SCARD_W_CANCELLED_BY_USER) {
          CPinDialogCNG::ResetPIN();
          throw UserCancelledException();
        }
        else if (result != ERROR_SUCCESS) {
          CPinDialogCNG::ResetPIN();
          throw TechnicalException("Signing failed: PIN/card error.");
        }
      }

      EstEID_log("Setting PIN to signer...");
      signer->setPin(CPinDialogCNG::GetPIN());

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

      EstEID_log("Signing...");
      string result = signer->sign();
      
      // append the signature to the comma separated signature list
      EstEID_log("Appending signature '%s'.", result.c_str());
      signatures += (signatures.length() ? "," + result : result);

      // cleanup
      delete signer; signer = NULL;

      // get the next hash string (or "" if nothing is left).
      EstEID_log("Getting next hash, hashPos=%d...", hashPos);
      currHash = getNextHash(hashString, hashPos, ",");

      // update progress bar
      if (progressBarDlg && hWndPB && !cancelSigning) {
        SendNotifyMessage(hWndPB, WM_UPDATE_PROGRESS, 0, 0);
      }
      // process pending Windows messages
      processMessages();
    }
    
    CPinDialogCNG::ResetPIN();

    if (cancelSigning) {
      EstEID_log("CNG mass signing failed, user canceled while signing hash %d of %d.", currentHash, hashCount);
      throw UserCancelledException("Signing was cancelled");
    }

    EstEID_log("%d hashes signed in %d seconds.", currentHash, (int)difftime(time(NULL), startTime));
    if (progressBarDlg) {
      if (progressBarDlg->IsWindow())
        progressBarDlg->DestroyWindow();
      delete progressBarDlg;
      progressBarDlg = NULL;
    }

    EstEID_log("All signatures: '%s'", signatures.c_str());

		*signature = _bstr_t(signatures.c_str()).Detach();
		clearErrors();
		EstEID_log("Signing ended");
		return S_OK;
	}
	catch (BaseException &e) {
		EstEID_log("Exception caught during signing: %s: %s", e.getErrorMessage().c_str(), e.getErrorDescription().c_str());
		setError(e);

    CPinDialogCNG::ResetPIN();
    if (progressBarDlg) {
      if (progressBarDlg->IsWindow())
        progressBarDlg->DestroyWindow();
      delete progressBarDlg;
      progressBarDlg = NULL;
    }

		return Error((this->errorMessage).c_str());
	}
}

BOOL CEstEIDIEPluginBHO::certificateMatchesId(PCCERT_CONTEXT certContext, BSTR id) {
	USES_CONVERSION;
	BYTE *cert;
	cert = (BYTE*)malloc(certContext->cbCertEncoded + 1);
	memcpy(cert, certContext->pbCertEncoded, certContext->cbCertEncoded);
	cert[certContext->cbCertEncoded] = '\0';
	std::string hashAsString;
	hashAsString = CEstEIDHelper::calculateMD5Hash((char*)cert);
	free(cert);
	BOOL result = (strcmp(hashAsString.c_str(), W2A(id)) == 0);
	EstEID_log("Cert match check result: %s", result ? "matches" : "does not match");
	return result;
}

void CEstEIDIEPluginBHO::clearErrors() {
	EstEID_log("");
	this->errorCode = 0;
	this->errorMessage.assign("");
}

void CEstEIDIEPluginBHO::setError(BaseException &exception) {
	EstEID_log("");
	this->errorCode = exception.getErrorCode();
	this->errorMessage.assign(exception.getErrorDescription());
	EstEID_log("Set error: %s (HEX %Xh, DEC %u)", this->errorMessage.c_str(), exception.getErrorCode(), exception.getErrorCode());
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
