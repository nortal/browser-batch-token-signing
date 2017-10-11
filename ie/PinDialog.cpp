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

#include "PinDialog.h"

#include "Labels.h"

#include <atlstr.h>

#define _L(KEY) Labels::l10n.get(KEY).c_str()

int PinDialog::attemptsRemaining = 3;
bool PinDialog::invalidPin = false;

std::wstring PinDialog::getWrongPinErrorMessage() {
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

std::wstring PinDialog::getEmptyPinErrorMessage() {
	return L"PIN on kohustuslik.";
}
LRESULT PinDialog::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
	CString rawPin;
	GetDlgItemText(IDC_PIN_FIELD, rawPin);
	pin = _strdup(ATL::CT2CA(rawPin));
	EndDialog(wID);
	return 0;
}

LRESULT PinDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	CAxDialogImpl<PinDialog>::OnInitDialog(uMsg, wParam, lParam, bHandled);
	ATLVERIFY(CenterWindow());
	GotoDlgCtrl(GetDlgItem(IDC_PIN_FIELD));
	SetDlgItemText(IDC_PIN_MESSAGE, label.c_str());
	SetDlgItemText(IDCANCEL, _L("cancel"));
	bHandled = TRUE;
	return FALSE; //Focus is set manually
}

LRESULT PinDialog::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
	EndDialog(wID);
	return 0;
}

char* PinDialog::getPin() {
	return pin;
}

void PinDialog::setAttemptsRemaining(int _attemptsRemaining) {
	attemptsRemaining = _attemptsRemaining;
}

void PinDialog::setInvalidPin(bool wasPinInvalid) {
  invalidPin = wasPinInvalid;
  if (wasPinInvalid) {
    attemptsRemaining--;
  }
  //updateControls();
}

// common controls
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif