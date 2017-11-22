/*
* Estonian ID card plugin for web browsers
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

#include <string>
#include "PinDialog.h"

int CPinDialog::attemptsRemaining = 3;
bool CPinDialog::invalidPin = false;

std::wstring CPinDialog::getWrongPinErrorMessage() {
	if (attemptsRemaining <= 0) {
		return L"PIN sisestati kolm korda valesti. PIN blokeeritud!";
	}

	std::wstring msg = L"";

	if (invalidPin) {
		msg = L"Vale PIN. ";	
	}

	else {
		msg = L"Palun sisestage PIN2. ";
	}

	if (attemptsRemaining < 3) {
		msg = msg + L"Teil on veel ";

		if (attemptsRemaining == 1) {
			msg = msg + L"1 katse jäänud!";
		}
		else {
			msg = msg + std::to_wstring(attemptsRemaining) + L" katset jäänud!";
		}
	}

	return msg;
}

std::wstring CPinDialog::getEmptyPinErrorMessage() {
  return L"PIN on kohustuslik.";
}

std::wstring CPinDialog::getShortPinErrorMessage() {
  return L"PIN peab olema 5 numbrit.";
}

int CPinDialog::getPin(char * pinOut, int pinLen) {
  // pinLen is not supposed to contain ending NUL!
  // Buffer pointed to by pinOut must be at least pinlen+1 characters long!
	
  INT_PTR nResponse = DoModal();
	if (attemptsRemaining <= 0) {
		EstEID_log("Pin blokeeritud");
    nResponse = PIN_ERROR_BLOCKED;
	}

	if (nResponse == IDOK) {
    strncpy(pinOut, pin, pinLen);
    pinOut[pinLen] = '\0';
	}
	else if (nResponse == IDCANCEL) {
		EstEID_log("Kasutaja katkestas.");
	}
  else if (nResponse == PIN_ERROR_BLOCKED) {
    EstEID_log("PIN blokeeritud.");
  }
  else if (nResponse < 0) {
    // Failed to create the dialog, error will be returned. Should not happen.
    EstEID_log("Failed to create PIN dialog, error: %d.", nResponse);
  }

  return nResponse;
}

void CPinDialog::setAttemptsRemaining(int _attemptsRemaining) {
	attemptsRemaining = _attemptsRemaining;
}

int CPinDialog::getAttemptsRemaining() {
  return attemptsRemaining;
}

void CPinDialog::setInvalidPin(bool wasPinInvalid) {
	invalidPin = wasPinInvalid;
  if (wasPinInvalid) {
    attemptsRemaining--;
  }
  updateControls();
}

