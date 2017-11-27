/*
 * Chrome Token Signing Native Host
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "NativeSigner.h"
#include "HostExceptions.h"
#include "Logger.h"

#include <Windows.h>
#include <ncrypt.h>
#include <WinCrypt.h>
#include <cryptuiapi.h>

#include <memory>

using namespace std;

SECURITY_STATUS NativeSigner::setPinForSigning(HCRYPTPROV_OR_NCRYPT_KEY_HANDLE key) {
	SECURITY_STATUS st = ERROR_SUCCESS;

	//// If we have already asked the PIN,
	//if (CPinDialogCNG::HasPIN()) {
	//  // then just pass it to CNG.
	//  if (!CPinDialogCNG::SetPIN()){
	//    return false;
	//  }
	//}
	//else {
	//  // else ask the PIN from the user, store it internally, and 
	//  // pass it to CNG to see if it is correct.
	//  if (!CPinDialogCNG::AskPIN(key)){
	//    return false;
	//  }
	//}

	if (pin != "") {
		WCHAR Pin[10] = { 0 };
		int pinLen = pin.length();
		MultiByteToWideChar(CP_ACP, 0, pin.c_str(), -1, (LPWSTR)Pin, pinLen);
		SECURITY_STATUS st = NCryptSetProperty(key, NCRYPT_PIN_PROPERTY, (PBYTE)Pin, (ULONG)wcslen(Pin) * sizeof(WCHAR), 0);
		if (st != ERROR_SUCCESS)
		{
			_log("**** Error 0x%x returned by NCryptSetProperty(NCRYPT_PIN_PROPERTY)\n", st);
			pin = ""; // reset stored pin
		}
	}

	return st;
}

NCRYPT_KEY_HANDLE NativeSigner::getCertificatePrivateKey(const vector<unsigned char> &digest, BOOL* freeKeyHandle) {
	BCRYPT_PKCS1_PADDING_INFO padInfo;
	DWORD obtainKeyStrategy = CRYPT_ACQUIRE_PREFER_NCRYPT_KEY_FLAG;

	ALG_ID alg = 0;

	switch (digest.size())
	{
	case BINARY_SHA1_LENGTH:
		padInfo.pszAlgId = NCRYPT_SHA1_ALGORITHM;
		alg = CALG_SHA1;
		break;
	case BINARY_SHA224_LENGTH:
		padInfo.pszAlgId = L"SHA224";
		obtainKeyStrategy = CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG;
		break;
	case BINARY_SHA256_LENGTH:
		padInfo.pszAlgId = NCRYPT_SHA256_ALGORITHM;
		alg = CALG_SHA_256;
		break;
	case BINARY_SHA384_LENGTH:
		padInfo.pszAlgId = NCRYPT_SHA384_ALGORITHM;
		alg = CALG_SHA_384;
		break;
	case BINARY_SHA512_LENGTH:
		padInfo.pszAlgId = NCRYPT_SHA512_ALGORITHM;
		alg = CALG_SHA_512;
		break;
	default:
		throw InvalidHashException();
	}

	SECURITY_STATUS err = 0;
	DWORD size = 256;
	vector<unsigned char> signature(size, 0);

	DWORD flags = obtainKeyStrategy | CRYPT_ACQUIRE_COMPARE_KEY_FLAG;
	DWORD spec = 0;
	//BOOL freeKeyHandle = false;
	HCRYPTPROV_OR_NCRYPT_KEY_HANDLE key = NULL;
	BOOL gotKey = true;
	HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_SYSTEM,
		X509_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG, L"MY");
	if (!store)
		throw TechnicalException("Failed to open Cert Store");

	PCCERT_CONTEXT certFromBinary = CertCreateCertificateContext(X509_ASN_ENCODING, cert.data(), cert.size());
	PCCERT_CONTEXT certInStore = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0, CERT_FIND_EXISTING, certFromBinary, 0);
	gotKey = CryptAcquireCertificatePrivateKey(certInStore, flags, 0, &key, &spec, freeKeyHandle);
	//CertFreeCertificateContext(certContext);

	return key;
}

vector<unsigned char> NativeSigner::sign(const vector<unsigned char> &digest)
{
	BCRYPT_PKCS1_PADDING_INFO padInfo;
	DWORD obtainKeyStrategy = CRYPT_ACQUIRE_PREFER_NCRYPT_KEY_FLAG;

	ALG_ID alg = 0;	
	switch (digest.size())
	{
	case BINARY_SHA1_LENGTH:
		padInfo.pszAlgId = NCRYPT_SHA1_ALGORITHM;
		alg = CALG_SHA1;
		break;
	case BINARY_SHA224_LENGTH:
		padInfo.pszAlgId = L"SHA224";
		obtainKeyStrategy = CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG;
		break;
	case BINARY_SHA256_LENGTH:
		padInfo.pszAlgId = NCRYPT_SHA256_ALGORITHM;
		alg = CALG_SHA_256;
		break;
	case BINARY_SHA384_LENGTH:
		padInfo.pszAlgId = NCRYPT_SHA384_ALGORITHM;
		alg = CALG_SHA_384;
		break;
	case BINARY_SHA512_LENGTH:
		padInfo.pszAlgId = NCRYPT_SHA512_ALGORITHM;
		alg = CALG_SHA_512;
		break;
	default:
		throw InvalidHashException();
	}
	
	HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_SYSTEM,
		X509_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG, L"MY");
	if (!store)
		throw TechnicalException("Failed to open Cert Store");
	
	PCCERT_CONTEXT certFromBinary = CertCreateCertificateContext(X509_ASN_ENCODING, cert.data(), cert.size());
	PCCERT_CONTEXT certInStore = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0, CERT_FIND_EXISTING, certFromBinary, 0);
	CertFreeCertificateContext(certFromBinary);
	CertCloseStore(store, 0);

	if (!certInStore)
		throw NoCertificatesException();

	DWORD flags = obtainKeyStrategy | CRYPT_ACQUIRE_COMPARE_KEY_FLAG;
	DWORD spec = 0;
	BOOL freeKeyHandle = false;
	HCRYPTPROV_OR_NCRYPT_KEY_HANDLE *handle = new HCRYPTPROV_OR_NCRYPT_KEY_HANDLE(0);
	BOOL gotKey = CryptAcquireCertificatePrivateKey(certInStore, flags, 0, handle, &spec, &freeKeyHandle);
	CertFreeCertificateContext(certInStore);
	if (!gotKey)
		throw NoCertificatesException();

	auto deleter = [=](HCRYPTPROV_OR_NCRYPT_KEY_HANDLE *key) {
		if (!freeKeyHandle)
			return;
		if (spec == CERT_NCRYPT_KEY_SPEC)
			NCryptFreeObject(*key);
		else
			CryptReleaseContext(*key, 0);
	};
	unique_ptr<HCRYPTPROV_OR_NCRYPT_KEY_HANDLE, decltype(deleter)> key(handle, deleter);

	SECURITY_STATUS err = ERROR_SUCCESS;
	vector<unsigned char> signature;
	switch (spec)
	{
	case CERT_NCRYPT_KEY_SPEC:
	{
		if (pin != "") {
			_log("Setting PIN for NCryptSignHash()...");
			setPinForSigning(*key);
		}

		DWORD size = 0;
		wstring algo(5, 0);
		err = NCryptGetProperty(*key.get(), NCRYPT_ALGORITHM_GROUP_PROPERTY, PBYTE(algo.data()), (algo.size() + 1) * 2, &size, 0);
		algo.resize(size / 2 - 1);
		bool isRSA = algo == L"RSA";
		
		err = NCryptSignHash(*key.get(), isRSA ? &padInfo : nullptr, PBYTE(digest.data()), DWORD(digest.size()),
			nullptr, 0, LPDWORD(&size), isRSA ? BCRYPT_PAD_PKCS1 : 0);
		if (FAILED(err))
			break;

		signature.resize(size);
		err = NCryptSignHash(*key.get(), isRSA ? &padInfo : nullptr, PBYTE(digest.data()), DWORD(digest.size()),
			signature.data(), DWORD(signature.size()), LPDWORD(&size), isRSA ? BCRYPT_PAD_PKCS1 : 0);
		break;
	}
	case AT_KEYEXCHANGE:
	case AT_SIGNATURE:
	{
		HCRYPTHASH hash = 0;
		if (!CryptCreateHash(*key.get(), alg, 0, 0, &hash))
			throw TechnicalException("CreateHash failed");
		if (!CryptSetHashParam(hash, HP_HASHVAL, digest.data(), 0))	{
			CryptDestroyHash(hash);
			throw TechnicalException("SetHashParam failed");
		}

		DWORD size = 0;
		if (!CryptSignHashW(hash, spec, nullptr, 0, nullptr, &size)) {
			CryptDestroyHash(hash);
			err = GetLastError();
			break;
		}

		signature.resize(size);
		if (!CryptSignHashW(hash, spec, nullptr, 0, LPBYTE(signature.data()), &size))
			err = GetLastError();
		CryptDestroyHash(hash);
		reverse(signature.begin(), signature.end());
		break;
	}
	default:
		throw TechnicalException("Incompatible key");
	}

	_log("sign return code: %x", err);
	switch (err)
	{
	case ERROR_SUCCESS:
		return signature;
	case SCARD_W_CANCELLED_BY_USER:
	case ERROR_CANCELLED:
		throw UserCancelledException("Signing was cancelled");
	case SCARD_W_CHV_BLOCKED:
		throw PinBlockedException();
	case NTE_INVALID_HANDLE:
		throw TechnicalException("The supplied handle is invalid");
	default:
		throw TechnicalException("Signing failed");
	}
}

