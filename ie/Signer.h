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

#pragma once

#include "stdafx.h"
#include "ProgressBar.h"

#include <string>
#include <ncrypt.h>
#include <WinCrypt.h>
#include <vector>

class Signer {
public:
	virtual ~Signer() = default;
	virtual NCRYPT_KEY_HANDLE getCertificatePrivateKey(const std::vector<unsigned char> &digest, BOOL* freeKeyHandle) { return NULL; }

	static Signer* createSigner(const std::vector<unsigned char> &cert, bool isBatchSigning);
	bool showInfo(const std::string &msg);
	virtual std::vector<unsigned char> sign(const std::vector<unsigned char> &digest) = 0;

	std::string getPin() {
		return pin;
	}

	void setPin(std::string _pin) {
		pin = _pin;
	}

	void setProgressBar(ProgressBar* _progressBar) {
		progressBar = _progressBar;
	}

protected:
	Signer(const std::vector<unsigned char> &_cert) : cert(_cert), progressBar(nullptr) {}

	std::string pin;
	std::vector<unsigned char> cert;
	ProgressBar* progressBar;
};
