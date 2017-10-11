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
#include "HostExceptions.h"
#include "PinDialog.h"
#include <atlstr.h>

CPinDialogCNG* CPinDialogCNG::s_dialog = NULL;
std::string CPinDialogCNG::s_pin = "";


bool CPinDialogCNG::HasPIN() {
  return (s_pin != "");
}

int CPinDialogCNG::AskPIN(NCRYPT_KEY_HANDLE hKey) {
  if (s_dialog == NULL) {
    s_dialog = new CPinDialogCNG(hKey);
    s_pin = "";
  }

  int result = ERROR_SUCCESS;
  INT_PTR nResponse = s_dialog->DoModal();
  if (nResponse == IDOK) {
    result = s_dialog->GetStatus();
  }
  else {
    result = SCARD_W_CANCELLED_BY_USER;
  }

  s_dialog->setInvalidPin(false);

  return result;
}

bool CPinDialogCNG::SetPIN() {
  bool result = false;
  if (s_dialog) {
    result = (s_dialog->setAndCheckPIN() == ERROR_SUCCESS);
  }
  return result;
}

void CPinDialogCNG::ResetPIN(void) {
  if (s_dialog) {
    s_dialog->releaseDialog();
  }
  s_pin = "";
}

void CPinDialogCNG::releaseDialog() {
  if (s_dialog) {
    delete s_dialog;
    s_dialog = NULL;
  }
}

LRESULT CPinDialogCNG::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
  CString rawPin;
  GetDlgItem(IDC_PIN_FIELD).GetWindowText(rawPin);

  // Check PIN length, do not return if zero.
  int len = rawPin.GetLength();
  if (len == 0) {
    std::wstring msg = getEmptyPinErrorMessage();
    SetDlgItemText(IDC_PIN_MESSAGE, &msg[0]);
    return -1;
  }

  // get the PIN from UI
  //pin = _strdup(ATL::CT2CA(rawPin));
  s_pin = ATL::CT2CA(rawPin);

  // pass the PIN to CNG
  setAndCheckPIN();

  // check the result
  if (m_status == ERROR_SUCCESS) {
	 s_dialog->setAttemptsRemaining(3);
  }
  else if (m_status == SCARD_W_WRONG_CHV) {
    // wrong PIN entered, retries left
    std::wstring msg = getWrongPinErrorMessage();
    SetDlgItemText(IDC_PIN_MESSAGE, &msg[0]);
    return -1;
  }
  else if (m_status == SCARD_W_CHV_BLOCKED) {
    // card blocked
    //throw PinBlockedException();
  }
  else {
    // other (fatal) error, cannot sign
    //throw TechnicalException("Signing failed because of PIN/card error.");
  }

  EndDialog(wID);
  return 0;
}

LRESULT CPinDialogCNG::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
  _log("CNG mass signing failed to ask PIN, user canceled.");
  //handleErrorWithCode(SCARD_W_CANCELLED_BY_USER, "AskPIN", m_pluginObj);
  
  EndDialog(wID);
  return 0;
}

int CPinDialogCNG::setAndCheckPIN(void) {
/* Return values:
*   ERROR_SUCCESS         Valid PIN (or PIN not checked)
*   SCARD_W_WRONG_CHV     Wrong PIN.
*   SCARD_W_CHV_BLOCKED   Card is blocked.
*   Other                 Other error (should be handled as fatal).
*/
  if (m_hKey) {
    // convert the PIN to Unicode
    WCHAR Pin[PIN2_LENGTH * 2] = { 0 };
    int pinLen = s_pin.length();
    MultiByteToWideChar(CP_ACP, 0, s_pin.c_str(), -1, (LPWSTR)Pin, pinLen);

    // pass the PIN to CNG
    m_status = NCryptSetProperty(m_hKey, NCRYPT_PIN_PROPERTY, (PBYTE)Pin, (ULONG)wcslen(Pin)*sizeof(WCHAR), 0);

    // check the result
    if (m_status == SCARD_W_WRONG_CHV) {
      // 0x8010006b: wrong pin
      _log("**** Error 0x%x returned by NCryptSetProperty(NCRYPT_PIN_PROPERTY): Wrong PIN.\n", m_status);
      this->setInvalidPin(true);
      s_pin = "";  // reset stored pin
    }
    else if (m_status == SCARD_W_CHV_BLOCKED) {
      // 0x8010006c: card is blocked
      _log("**** Error 0x%x returned by NCryptSetProperty(NCRYPT_PIN_PROPERTY): Card blocked.\n", m_status);
    }
    else if (m_status != ERROR_SUCCESS)
    {
      // other error
      _log("**** Error 0x%x returned by NCryptSetProperty(NCRYPT_PIN_PROPERTY).\n", m_status);
    }
  }
  else {
    // PIN not checked, signing may ask PIN with the default dialog.
    m_status = ERROR_SUCCESS;
  }

  return m_status;
}