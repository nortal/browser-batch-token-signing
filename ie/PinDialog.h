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

#include "Logger.h"
#include "resource.h"

#include <atlbase.h>
#include <atlhost.h>
#include <atlstr.h>
#include <atlctl.h>
#include <string>

class PinDialog : public CAxDialogImpl<PinDialog>
{
public:
	PinDialog(const std::wstring &_label) : label(_label) {}
	char* getPin();
	void setAttemptsRemaining(int attemptsRemaining);
	void setInvalidPin(bool wasPinInvalid);

	// Dialog Data
	enum { IDD = IDD_PIN_DIALOG };

protected:
	BEGIN_MSG_MAP(PinDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
		CHAIN_MSG_MAP(CAxDialogImpl<PinDialog>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {	
		if (GetDlgItem(IDC_PIN_MESSAGE).m_hWnd == (HWND)lParam && (invalidPin || attemptsRemaining < 3)) {
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
		}

		SetBkColor((HDC)wParam, RGB(240, 240, 240));

		HBRUSH  hBr = (HBRUSH)CreateSolidBrush(RGB(240, 240, 240));
		return (LRESULT)hBr;
	}

  void updateControls() {
    GotoDlgCtrl(GetDlgItem(IDC_PIN_FIELD));
    if (invalidPin || attemptsRemaining < 3) {
      std::wstring msg = getWrongPinErrorMessage();
      _log("Sul on %i katset jäänud", attemptsRemaining);
      SetDlgItemText(IDC_PIN_MESSAGE, &msg[0]);
    }

	SetDlgItemText(IDC_PIN_FIELD, L"");

    // Disable text field and OK button of no more tries left. Set the focus to Cancel button.
    if (attemptsRemaining <= 0) {
	  GetDlgItem(IDC_PIN_FIELD).EnableWindow(FALSE);
      GetDlgItem(IDOK).EnableWindow(FALSE);
      GetDlgItem(IDCANCEL).SetFocus();
    }

    // make the window topmost and set focus to it
    SetForegroundWindow(m_hWnd);
  }

protected:
  static int attemptsRemaining;
  static bool invalidPin;
  char* pin;
  int pinLen;
  std::wstring label;
  std::wstring getWrongPinErrorMessage();
  std::wstring getEmptyPinErrorMessage();
};
