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
#pragma once

#include "resource.h"
#include <atlbase.h>
#include <atlhost.h>
#include <atlstr.h>
#include <atlctl.h>
#include <string>
#include "plugin-class.h"
#include "esteid_log.h"

using namespace ATL;

extern int attemptsRemaining;

#define PIN_ERROR_GENERAL   -1
#define PIN_ERROR_BLOCKED   -2
#define PIN_ERROR_EMPTY_PIN -3

#ifndef PIN2_LENGTH          
#define PIN2_LENGTH          5
#endif

class CPinDialog : 
	public CAxDialogImpl<CPinDialog>
{
public:
  int getPin(char*, int);
	void showPinBlocked();
	void setAttemptsRemaining(int attemptsRemaining);
  int getAttemptsRemaining();
	void setInvalidPin(bool wasPinInvalid);
  void setPinMessage(char*);
  CPinDialog(){ pinLen = PIN2_LENGTH; }
	~CPinDialog(){}

  enum { IDD = IDD_PINDIALOG };

  BEGIN_MSG_MAP(CPinDialog)
	  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	  MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
	  COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
	  COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
	  CHAIN_MSG_MAP(CAxDialogImpl<CPinDialog>)
  END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {		
		CAxDialogImpl<CPinDialog>::OnInitDialog(uMsg, wParam, lParam, bHandled);
		ATLVERIFY(CenterWindow());
		GotoDlgCtrl(GetDlgItem(IDC_PIN_ENTRY));

		if (invalidPin || attemptsRemaining < 3) {			
			std::wstring msg = getWrongPinErrorMessage();
			EstEID_log("sul on %i katseid jäänud", attemptsRemaining);
			SetDlgItemText(IDC_PIN_MESSAGE, &msg[0]);
		}

    // Disable text field and OK button of no more tries left. Set the focus to Cancel button.
    if (attemptsRemaining <= 0) {
      GetDlgItem(IDC_PIN_ENTRY).EnableWindow(FALSE);
      GetDlgItem(IDOK).EnableWindow(FALSE);
      GetDlgItem(IDCANCEL).SetFocus();
    }

    // make the window topmost and set focus to it
    SetForegroundWindow(m_hWnd);
    
    bHandled = TRUE;
	return FALSE; //Focus is set manually
	}

	LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {	
		if (GetDlgItem(IDC_PIN_MESSAGE).m_hWnd == (HWND)lParam && (invalidPin || attemptsRemaining < 3)) {
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
		}

		SetBkColor((HDC)wParam, RGB(240, 240, 240));

		HBRUSH  hBr = (HBRUSH)CreateSolidBrush(RGB(240, 240, 240));
		return (LRESULT)hBr;
	}

	LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {	
		CString rawPin;
		GetDlgItem(IDC_PIN_ENTRY).GetWindowText(rawPin);
    
		// Check PIN length, do not return if invalid.
		int len = rawPin.GetLength();
		if (len == 0) {
		  std::wstring msg = getEmptyPinErrorMessage();
		  SetDlgItemText(IDC_PIN_MESSAGE, &msg[0]);
		  return -1;
		}

		pin = _strdup(ATL::CT2CA(rawPin));
		EndDialog(wID);
		return 0;
	}

	LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
		EndDialog(wID);
		return 0;
	}

  void setPinMessage(std::wstring msg) {
    SetDlgItemText(IDC_PIN_MESSAGE, &msg[0]);
  }

  void updateControls() {
    GotoDlgCtrl(GetDlgItem(IDC_PIN_ENTRY));
    if (invalidPin || attemptsRemaining < 3) {
      std::wstring msg = getWrongPinErrorMessage();
      EstEID_log("sul on %i katset jäänud", attemptsRemaining);
      SetDlgItemText(IDC_PIN_MESSAGE, &msg[0]);
    }

	SetDlgItemText(IDC_PIN_ENTRY, L"");

    // Disable text field and OK button of no more tries left. Set the focus to Cancel button.
    if (attemptsRemaining <= 0) {
      GetDlgItem(IDC_PIN_ENTRY).EnableWindow(FALSE);
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
  std::wstring getWrongPinErrorMessage();
  std::wstring getEmptyPinErrorMessage();
  std::wstring getShortPinErrorMessage();
};

class CPinDialogCNG : public CPinDialog {
private:
  static CPinDialogCNG* s_dialog;
  static std::string    s_pin;

  PluginInstance*       m_pluginObj;
  NCRYPT_KEY_HANDLE     m_hKey;
  SECURITY_STATUS       m_status;
  int                   m_attemptsRemaining;

  // hide the default constructor
  CPinDialogCNG() {}

  int   setAndCheckPIN();
  void  releaseDialog();
  
public:
  static bool HasPIN();
  static bool AskPIN(PluginInstance* obj, NCRYPT_KEY_HANDLE hKey);
  static bool SetPIN();
  static void ResetPIN();

  BEGIN_MSG_MAP(CPinDialogCNG)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
    COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
    COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
  END_MSG_MAP()

  // the only allowed constructor
  CPinDialogCNG(PluginInstance* obj, NCRYPT_KEY_HANDLE hKey) {
    m_pluginObj = obj;
    m_hKey = hKey;
    m_status = -1;
    m_attemptsRemaining = 3;
  }

  // message handlers
  LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  bool IsError() {
    return (m_status != ERROR_SUCCESS);
  }
  bool IsPinValid() {
    return (m_status == ERROR_SUCCESS);
  }
  bool IsCardBlocked() {
    return (m_status == SCARD_W_CHV_BLOCKED);
  }

};
