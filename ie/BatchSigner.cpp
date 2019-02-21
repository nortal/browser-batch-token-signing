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

#include "BatchSigner.h"
#include "BinaryUtils.h"
#include "Exceptions.h"
#include "HashListParser.h"
#include "Labels.h"
#include "Logger.h"
#include "ProgressBar.h"
#include "Signer.h"
#include "SigningPinDialog.h"

using namespace std;

BatchSigner::BatchSigner(const vector<unsigned char>& cert)
{
	this->cert = cert;
}

int setAndCheckPIN(string pin, NCRYPT_KEY_HANDLE key) {
	/* Return values:
	*   ERROR_SUCCESS         Valid PIN (or PIN not checked)
	*   SCARD_W_WRONG_CHV     Wrong PIN.
	*   SCARD_W_CHV_BLOCKED   Card is blocked.
	*   Other                 Other error (should be handled as fatal).
	*/
	int status;
	if (key) {
		// convert the PIN to Unicode
		WCHAR Pin[PIN2_LENGTH + 1] = { 0 };

		MultiByteToWideChar(CP_ACP, 0, pin.c_str(), -1, (LPWSTR)Pin, PIN2_LENGTH + 1);

		status = NCryptSetProperty(key, NCRYPT_PIN_PROPERTY, (PBYTE)Pin, (ULONG)wcslen(Pin) * sizeof(WCHAR), 0);

		// check the result
		if (status == SCARD_W_WRONG_CHV) {
			// 0x8010006b: wrong pin
			_log("**** Error 0x%x returned by NCryptSetProperty(NCRYPT_PIN_PROPERTY): Wrong PIN.\n", status);
		}
		else if (status == SCARD_W_CHV_BLOCKED) {
			// 0x8010006c: card is blocked
			_log("**** Error 0x%x returned by NCryptSetProperty(NCRYPT_PIN_PROPERTY): Card blocked.\n", status);
		}
		else if (status != ERROR_SUCCESS)
		{
			// other error
			_log("**** Error 0x%x returned by NCryptSetProperty(NCRYPT_PIN_PROPERTY).\n", status);
		}
		else
		{
			// Note that in case the card driver specifies PIN_CACHE_POLICY_TYPE = PinCacheAlwaysPrompt (like the IDEMIA driver),
			// ERROR_SUCCESS is returned without verifying and caching the PIN.
			// Verification should then be handled by the signer class.
			_log("Successfully set NCRYPT_PIN_PROPERTY.", status);
		}
	}
	else {
		_log("PIN not checked, signing may ask PIN with the default dialog.");
		status = ERROR_SUCCESS;
	}
	return status;
}

string askPin(Signer& signer, const vector<unsigned char>& hash) {
	_log("Showing pin entry dialog");

	wstring label = Labels::l10n.get("sign PIN");
	size_t start_pos = 0;
	while ((start_pos = label.find(L"@PIN@", start_pos)) != std::string::npos) {
		label.replace(start_pos, 5, L"PIN");
		start_pos += 3;
	}

	bool isInitialCheck = true;
	int attemptsRemaining = 3;
	do {
		wstring msg;
		if (attemptsRemaining < 3)
		{
			if (!isInitialCheck)
				msg = Labels::l10n.get("incorrect PIN2");
			msg += Labels::l10n.get("tries left") + L" " + to_wstring(attemptsRemaining);
		}

		string pin = SigningPinDialog::getPin(label, msg);
		if (pin.empty()) {
			_log("User cancelled");
			throw UserCancelledException();
		}

		signer.setPin(pin);

		BOOL freeKeyHandle = false;
		NCRYPT_KEY_HANDLE key = signer.getCertificatePrivateKey(hash, &freeKeyHandle);
		int status = setAndCheckPIN(pin, key);
		if (freeKeyHandle) NCryptFreeObject(key);

		if (status == ERROR_SUCCESS) {
			return pin;
		}

		if (status == SCARD_W_WRONG_CHV) {
			_log("Wrong PIN2");
			attemptsRemaining--;
			isInitialCheck = false;
			continue;
		}
		if (status == SCARD_W_CHV_BLOCKED) {
			MessageBox(nullptr, Labels::l10n.get("PIN2 blocked").c_str(), L"PIN Blocked", MB_OK | MB_ICONERROR);
			_log("PIN2 blocked");
			throw PinBlockedException();
		}
		if (status == SCARD_W_CANCELLED_BY_USER) {
			_log("User cancelled");
			throw UserCancelledException();
		}
		throw TechnicalException("Signing failed: PIN/card error.");
	} while (true);
}

vector<vector<unsigned char>> BatchSigner::sign(string hashList, string info)
{
	vector<vector<unsigned char>> hashes = HashListParser::parse(hashList);

	int hashIndex = 0;
	size_t hashLength = 0;
	string pin = "";
	vector<vector<unsigned char>> signatures;
	time_t startTime;
	ProgressBar* progressBar = 0;

	time(&startTime);

	try
	{
		vector<vector<unsigned char>>::iterator hash = hashes.begin();
		while (hash != hashes.end()) {
			hashIndex++;
			_log("Signing hash %d of %d", hashIndex, hashes.size());

			unique_ptr<Signer> signer(Signer::createSigner(cert));

			if (hashIndex == 1)
			{
				if (!signer->showInfo(info))
					throw UserCancelledException();

				hashLength = hash->size();
				pin = askPin(*signer, *hash);

				if (hashes.size() > 2)
				{
					progressBar = new ProgressBar(hashes.size());
				}
			}
			else if (hash->size() != hashLength)
			{
				_log("All hashes must have the same size for batch signing.");
				throw InvalidHashException();
			}

			if (progressBar) {
				if (progressBar->shouldCancel())
				{
					throw UserCancelledException();
				}
				progressBar->updateProgress();
			}

			_log("Setting PIN to signer...");
			signer->setPin(pin);

			vector<unsigned char> signature = signer->sign(*hash);

			// Check if PIN was updated during signing
			if (signer->getPin() != pin) {
				pin = signer->getPin();
			}

			// append the signature to comma separated signature list
			_log("Appending signature '%s'.", BinaryUtils::bin2hex(signature).c_str());
			signatures.push_back(signature);

			hash = next(hash);
		}

		_log("%d hashes signed in %d seconds.", hashes.size(), (int)difftime(time(NULL), startTime));

		if (progressBar) {
			if (progressBar->shouldCancel())
			{
				throw UserCancelledException();
			}
			delete progressBar;
		}
	}
	catch (const BaseException &e)
	{
		if (progressBar) {
			delete progressBar;
		}
		throw e;
	}

	return signatures;
}
