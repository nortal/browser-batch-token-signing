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
#include "Labels.h"
#include "ProgressBar.h"
#include "ProgressBarResource.h"

DWORD WINAPI ProgressBar::DialogThreadFunction(LPVOID lpParam)
{
	ProgressBar* progressBar = (ProgressBar*)lpParam;

	HMODULE hModule = ATL::_AtlBaseModule.GetModuleInstance();
	HWND hwndDlg = CreateDialogParam(hModule, MAKEINTRESOURCE(IDD_PROGRESS_BAR_DIALOG), 0, DlgProc, (LPARAM)progressBar);
	progressBar->initializeDialogHandle(hwndDlg);

	ShowWindow(hwndDlg, SW_SHOWNORMAL);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

ProgressBar::ProgressBar(int numberOfItems)
{
	m_numberOfItems = numberOfItems;
	m_currentItem = 0;
	m_shouldCancel = false;

	m_initializedEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("InitializedEvent"));

	DWORD threadId;
	CreateThread(
		NULL,
		0,
		&DialogThreadFunction,
		(LPVOID)this,
		0,
		&threadId
	);

	// Wait until window is initialized so that messages can be sent to it
	WaitForSingleObject(m_initializedEvent, INFINITE);
	CloseHandle(m_initializedEvent);
}

ProgressBar::~ProgressBar()
{
	if (m_hwndDlg) {
		EndDialog(m_hwndDlg, IDOK);
		m_hwndDlg = NULL;
	}
}

void ProgressBar::initializeDialogHandle(HWND hwndDlg)
{
	m_hwndDlg = hwndDlg;
	SetEvent(m_initializedEvent);
}

void ProgressBar::updateProgress() {

	if (m_currentItem < m_numberOfItems) {
		m_currentItem++;
	}

	WCHAR buf[100] = { 0 };
	wsprintf(buf, Labels::l10n.get("batch sign create").c_str(), m_currentItem, m_numberOfItems);
	SetDlgItemText(m_hwndDlg, IDC_PROGRESS_TEXT, buf);

	SendDlgItemMessage(m_hwndDlg, IDC_PROGRESS_BAR, PBM_STEPIT, 0, 0);
}

bool ProgressBar::shouldCancel()
{
	return m_shouldCancel;
}

INT_PTR CALLBACK ProgressBar::DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		ProgressBar* self = (ProgressBar*)lParam;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		SetWindowText(hwndDlg, Labels::l10n.get("batch signing").c_str());

		// Set progress bar range. LOWORD contains the min value, HIWORD the max value.
		LPARAM range = MAKELONG(0, self->m_numberOfItems);
		SendDlgItemMessage(hwndDlg, IDC_PROGRESS_BAR, PBM_SETRANGE, 0, range);

		// Set progress bar step size to 1
		SendDlgItemMessage(hwndDlg, IDC_PROGRESS_BAR, PBM_SETSTEP, 1, 0);

		SetDlgItemText(hwndDlg, IDCANCEL, Labels::l10n.get("cancel").c_str());

		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			ProgressBar* self = (ProgressBar*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
			self->m_shouldCancel = true;
			BOOL result = EndDialog(hwndDlg, IDCANCEL);
			PostQuitMessage(0);
			return result;
		}
		return FALSE;
	}
	return FALSE;
}
