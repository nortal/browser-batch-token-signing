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

#include "PinDialog.h"

#ifndef PIN2_LENGTH          
#define PIN2_LENGTH          5
#endif

class PinDialogCNG : public PinDialog {
private:
  static PinDialogCNG*  s_dialog;
  static std::string    s_pin;

  NCRYPT_KEY_HANDLE     m_hKey;
  SECURITY_STATUS       m_status;
  int                   m_attemptsRemaining;

  // hide the default constructor
  PinDialogCNG() : PinDialog(L"") {}

  int   setAndCheckPIN();
  void  releaseDialog();

public:
  static bool HasPIN();
  static int  AskPIN(NCRYPT_KEY_HANDLE hKey);
  static bool SetPIN();
  static void ResetPIN();
  static std::string GetPIN() { return s_pin; }

  NCRYPT_KEY_HANDLE GetKey()          { return m_hKey; }
  void SetKey(NCRYPT_KEY_HANDLE key)  { m_hKey = key;  }

  BEGIN_MSG_MAP(CPinDialogCNG)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
    COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
    COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
  END_MSG_MAP()

  // the only allowed constructor
  PinDialogCNG(NCRYPT_KEY_HANDLE hKey);

  // message handlers
  LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  int GetStatus() {
    return m_status;
  }

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
