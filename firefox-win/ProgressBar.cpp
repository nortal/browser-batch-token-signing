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

#include "resource.h"
#include <atlbase.h>
#include <atlhost.h>
#include <atlstr.h>
#include <atlctl.h>
#include <string>
#include "plugin-class.h"
#include "esteid_log.h"
#include "ProgressBar.h"

using namespace ATL;

extern bool cancelSigning; // located in plugin-class.c

LRESULT CProgressBarDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  CAxDialogImpl<CProgressBarDialog>::OnInitDialog(uMsg, wParam, lParam, bHandled);
  ATLVERIFY(CenterWindow());
  
  // Set progress bar range. LOWORD contains the min value, HIWORD the max value.
  LPARAM range = MAKELONG(0, m_numberOfItems);
  ::SendDlgItemMessage(m_hWnd, IDC_PROGRESS_BAR, PBM_SETRANGE, 0, range);

  // Set progress bar step size to 1
  ::SendDlgItemMessage(m_hWnd, IDC_PROGRESS_BAR, PBM_SETSTEP, 1, 0);

  // make the window topmost and set focus to it
  SetForegroundWindow(m_hWnd);
  GotoDlgCtrl(GetDlgItem(IDCANCEL));

  bHandled = TRUE;
  return FALSE; //Focus is set manually
}

LRESULT CProgressBarDialog::OnUpdateProgress(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

  if (m_currentItem < m_numberOfItems) {
    m_currentItem++;
  }
  updateProgressBarMessage();
  updateProgressBar();

  bHandled = TRUE;
  return 0;
}

LRESULT CProgressBarDialog::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
  EstEID_log("User canceled while signing hash %d of %d.", m_currentItem, m_numberOfItems);
  cancelSigning = true;
  DestroyWindow();
  return 0;
}

void CProgressBarDialog::updateProgressBarMessage() {
  WCHAR buf[100] = { 0 };
  wsprintf(buf, L"Creating signature %d of %d...", m_currentItem, m_numberOfItems);
  SetDlgItemText(IDC_PROGRESS_TEXT, buf);

  Invalidate();
}

void CProgressBarDialog::updateProgressBar() {
  ::SendDlgItemMessage(m_hWnd, IDC_PROGRESS_BAR, PBM_STEPIT, 0, 0);
  SetDlgItemText(IDCANCEL, L"Cancel");
}
