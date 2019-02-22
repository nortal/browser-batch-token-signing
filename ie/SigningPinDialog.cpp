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

#include "SigningPinDialog.h"

#include "Labels.h"
#include "PinDialogResource.h"

std::string SigningPinDialog::getPin(const std::wstring &label, const std::wstring &message, HWND pParent)
{
	SigningPinDialog p;
	p.label = label;
	p.message = message;
	if (DialogBoxParam(ATL::_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDD_PIN_DIALOG), pParent, DlgProc, LPARAM(&p)) != IDOK)
		p.pin.clear();
	auto err = GetLastError();
	return p.pin;
}

INT_PTR CALLBACK SigningPinDialog::DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		SigningPinDialog* self = (SigningPinDialog*)lParam;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		SetDlgItemText(hwndDlg, IDC_MESSAGE, self->message.c_str());
		SetDlgItemText(hwndDlg, IDC_LABEL, self->label.c_str());
		SetDlgItemText(hwndDlg, IDCANCEL, Labels::l10n.get("cancel").c_str());
		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_PIN_FIELD:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				WORD len = WORD(SendDlgItemMessage(hwndDlg, IDC_PIN_FIELD, EM_LINELENGTH, 0, 0));
				EnableWindow(GetDlgItem(hwndDlg, IDOK), len == PIN2_LENGTH);
				SendMessage(hwndDlg, DM_SETDEFID, len == PIN2_LENGTH ? IDOK : IDCANCEL, 0);
			}
			break;
		case IDOK:
		{
			size_t len = size_t(SendDlgItemMessage(hwndDlg, IDC_PIN_FIELD, EM_LINELENGTH, 0, 0));
			std::wstring tmp(len + 1, 0);
			GetDlgItemText(hwndDlg, IDC_PIN_FIELD, &tmp[0], tmp.size());
			SigningPinDialog* self = (SigningPinDialog*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
			self->pin = std::string(tmp.cbegin(), tmp.cend());
			return EndDialog(hwndDlg, IDOK);
		}
		case IDCANCEL:
			return EndDialog(hwndDlg, IDCANCEL);
		}
		return FALSE;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_MESSAGE))
		{
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
			SetBkMode((HDC)wParam, TRANSPARENT);
			SigningPinDialog* self = (SigningPinDialog*)lParam;
			return (INT_PTR)GetSysColorBrush(CTLCOLOR_DLG);
		}
		return FALSE;
	}
	return FALSE;
}
