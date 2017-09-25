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

#include "Logger.h"
#include "resource.h"
#include <atlbase.h>
#include <atlhost.h>
#include <atlstr.h>
#include <atlctl.h>
#include <string>

using namespace ATL;

#define WM_UPDATE_PROGRESS    (WM_USER + 0x0001)

class CProgressBarDialog : 
  public CAxDialogImpl<CProgressBarDialog>
{
private:
  int m_numberOfItems;
  int m_currentItem;
  
public:
  CProgressBarDialog(int numberOfItems) {
    m_numberOfItems = numberOfItems;
    m_currentItem = 0;
  }
  ~CProgressBarDialog(){}

  enum { IDD = IDD_PROGRESSBARDLG };

  BEGIN_MSG_MAP(CProgressBarDialog)
	  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_UPDATE_PROGRESS, OnUpdateProgress)
    COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
    CHAIN_MSG_MAP(CAxDialogImpl<CProgressBarDialog>)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnUpdateProgress(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
  void updateProgressBarMessage();
  void updateProgressBar();

};


