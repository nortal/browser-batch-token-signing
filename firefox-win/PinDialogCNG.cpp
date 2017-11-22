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

CPinDialogCNG* CPinDialogCNG::s_dialog = NULL;
std::string CPinDialogCNG::s_pin = "";

bool CPinDialogCNG::HasPIN() {
  return (s_pin != "");
}

bool CPinDialogCNG::AskPIN(PluginInstance* obj, NCRYPT_KEY_HANDLE hKey) {
  if (s_dialog == NULL) {
    s_dialog = new CPinDialogCNG(obj, hKey);
    s_pin = "";
  }

  // ask the PIN from the user
  bool result = false;
  INT_PTR nResponse = s_dialog->DoModal();
  if (nResponse == IDOK) {
    // check what we've got
    result = s_dialog->IsPinValid();
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
  GetDlgItem(IDC_PIN_ENTRY).GetWindowText(rawPin);

  // Check PIN length, do not return if zero.
  // TODO: Drop this and try to use empty PIN ?
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
  int status = setAndCheckPIN();

  // check the result
  if (status == ERROR_SUCCESS) {
	s_dialog->setAttemptsRemaining(3);
  }
  else if (status == SCARD_W_WRONG_CHV) {
    // wrong PIN entered, retries left
    std::wstring msg = getWrongPinErrorMessage();
    SetDlgItemText(IDC_PIN_MESSAGE, &msg[0]);
    return -1;
  }
  else {
    // card blocked or fatal error, cannot sign
  }

  EndDialog(wID);
  return 0;
}

LRESULT CPinDialogCNG::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
  EstEID_log("CNG mass signing failed to ask PIN, user canceled.");
  handleErrorWithCode(SCARD_W_CANCELLED_BY_USER, "AskPIN", m_pluginObj);
  s_pin = "";

  EndDialog(wID);
  return 0;
}

int CPinDialogCNG::setAndCheckPIN(void) {
/* Return values:
*   ERROR_SUCCESS         Valid PIN.
*   SCARD_W_WRONG_CHV     Wrong PIN.
*   SCARD_W_CHV_BLOCKED   Card is blocked.
*   Other                 Other error (should be handled as fatal).
*/
  
  // convert the PIN to Unicode
  WCHAR Pin[PIN2_LENGTH*2] = { 0 };
  int pinLen = s_pin.length();
  MultiByteToWideChar(CP_ACP, 0, s_pin.c_str(), -1, (LPWSTR)Pin, pinLen);
  
  // pass the PIN to CNG
  m_status = NCryptSetProperty(m_hKey, NCRYPT_PIN_PROPERTY, (PBYTE)Pin, (ULONG)wcslen(Pin)*sizeof(WCHAR), 0);
  
  // check the result
  if (m_status == SCARD_W_WRONG_CHV) {
    // 0x8010006b: wrong pin
    EstEID_log("**** Error 0x%x returned by NCryptSetProperty(NCRYPT_PIN_PROPERTY): Wrong PIN.\n", m_status);
    this->setInvalidPin(true);
    s_pin = "";
  }
  else if (m_status == SCARD_W_CHV_BLOCKED) {
    // 0x8010006c: card is blocked
    EstEID_log("**** Error 0x%x returned by NCryptSetProperty(NCRYPT_PIN_PROPERTY): Card blocked.\n", m_status);
    handleErrorWithCode(m_status, "NCryptSetProperty", m_pluginObj);
    return false;
  }
  else if (m_status != ERROR_SUCCESS)
  {
    // other error
    EstEID_log("**** Error 0x%x returned by NCryptSetProperty(NCRYPT_PIN_PROPERTY).\n", m_status);
    handleErrorWithCode(m_status, "NCryptSetProperty", m_pluginObj);
    return false;
  }

  return m_status;
}