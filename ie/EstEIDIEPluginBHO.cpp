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

#include "BatchSigner.h"
#include "BinaryUtils.h"
#include "CertificateSelector.h"
#include "Labels.h"
#include "Logger.h"
#include "Signer.h"
#include "version.h"

#include <memory>
#include <string.h>

using namespace std;

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

	// convert OLE string to std::string for easier handling
	wstring hashToSign(hashhex, SysStringLen(hashhex));
	std::string hashList(hashToSign.begin(), hashToSign.end());

	try {
		isSiteAllowed();

		CComBSTR idHex;
		cert->get_id(&idHex);
		if (idHex != id)
			throw NoCertificatesException();

		language = _bstr_t(_language).Detach();
		Labels::l10n.setLanguage(W2A(language));

		CComBSTR certHex;
		cert->get_cert(&certHex);
		vector<unsigned char> certBin = BinaryUtils::hex2bin(W2A(certHex));
		BatchSigner batchSigner(certBin);

		string infoString("");
		if (info.vt == VT_BSTR)
		{
			infoString = W2A_CP(info.bstrVal, CP_UTF8);
		}

		vector<vector<unsigned char>> signatures = batchSigner.sign(hashList, infoString);
		string signaturesHex;
		vector<vector<unsigned char>>::iterator sig = signatures.begin();
		while (sig != signatures.end()) {
			string signatureHex = BinaryUtils::bin2hex(*sig);
			signaturesHex += (signaturesHex.length() ? "," + signatureHex : signatureHex);
			sig = next(sig);
		}

		*signature = _bstr_t(signaturesHex.c_str()).Detach();

		clearErrors();
		_log("Signing ended");
		return S_OK;
	}
	catch (const BaseException &e) {
		_log("Exception caught during signing: %s", e.what());
		setError(e);
		return Error(errorMessage.c_str());
	}
}

void CEstEIDIEPluginBHO::clearErrors() {
	_log("");
	errorCode = 0;
	errorMessage.clear();
}

void CEstEIDIEPluginBHO::setError(const BaseException &exception) {
	errorCode = exception.toInt();
	errorMessage.assign(exception.what());
	_log("Set error: %s (HEX %Xh, DEC %u)", errorMessage.c_str(), errorCode, errorCode);
}
