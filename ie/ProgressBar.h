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

#include <Windows.h>

class ProgressBar
{
private:
	int m_numberOfItems;
	int m_currentItem;
	volatile bool m_shouldCancel;
	HANDLE m_initializedEvent;
	HWND m_hwndDlg;

public:
	ProgressBar(int numberOfItems);
	~ProgressBar();

	void initializeDialogHandle(HWND hwndDlg);
	void updateProgress();
	bool shouldCancel();

	void show();
	void hide();

private:
	static DWORD WINAPI DialogThreadFunction(LPVOID lpParam);
	static INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
