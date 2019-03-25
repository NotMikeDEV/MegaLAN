#include "stdafx.h"
#include <string>
#include <vector>
#include "commctrl.h"
#include "Windows.h"
#include <shellapi.h>
#include "MegaLAN.h"
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

NICSelector::NICSelector()
{
	Startup = true;
}


NICSelector::~NICSelector()
{
}
struct NIC NICSelector::Run()
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SELECT_NIC), hWnd, NICSelector::SelectNICWndProc, (LPARAM)this);
	Startup = false;
	const LPCWSTR Software = L"Software";
	HKEY Key, MyKey;
	RegOpenKey(HKEY_CURRENT_USER, Software, &Key);
	RegCreateKeyEx(Key, L"MegaLAN", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &MyKey, NULL);
	RegSetKeyValue(MyKey, NULL, L"SelectedNIC", REG_SZ, SelectedNIC.ID.c_str(), SelectedNIC.ID.length()*2 + 2);
	RegCloseKey(Key);
	RegCloseKey(MyKey);
	return SelectedNIC;
}

INT_PTR CALLBACK NICSelector::SelectNICWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		NICSelector* Me = (NICSelector*)lParam;
		if (Me)
		{
			SetWindowLongPtr(hDlg, DWLP_USER, lParam);
		}
		SendMessage(hDlg, WM_USER + 1, 0, 0);
		if (Me->NICList.size() == 0 && Me->Startup)
		{
			SendMessage(hDlg, WM_COMMAND, IDC_ADD_NIC, 0);
		}
		if (Me->NICList.size() == 1 && Me->Startup)
		{
			Me->SelectedNIC = Me->NICList[0];
			EndDialog(hDlg, 0);
		}
		else if (Me->Startup)
		{
			const LPCWSTR Software = L"Software\\MegaLAN";
			HKEY Key;
			RegOpenKey(HKEY_CURRENT_USER, Software, &Key);
			WCHAR* SelectedNIC = new WCHAR[MAX_VALUE_NAME];
			memset(SelectedNIC, 0, sizeof(WCHAR[MAX_VALUE_NAME]));
			DWORD SelectedNICLen = MAX_VALUE_NAME;
			RegGetValue(Key, NULL, L"SelectedNIC", RRF_RT_REG_SZ, NULL, SelectedNIC, &SelectedNICLen);
			RegCloseKey(Key);
			for (struct NIC& Interface : Me->NICList)
			{
				if (SelectedNICLen && Interface.ID.compare(SelectedNIC) == 0)
				{
					Me->SelectedNIC = Interface;
					delete[] SelectedNIC;
					EndDialog(hDlg, 0);
					return TRUE;
				}
			}
			delete[] SelectedNIC;
		}
	}
	break;
	case WM_USER + 1:
	{
		NICSelector* Me = (NICSelector*)GetWindowLongPtr(hDlg, DWLP_USER);
		HWND hwndList = GetDlgItem(hDlg, IDC_LIST_NICS);
		int pos = SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
		const LPCWSTR AdapterKey = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
		HKEY Key;
		RegOpenKey(HKEY_LOCAL_MACHINE, AdapterKey, &Key);
		DWORD valueCount;
		LONG retCode = RegQueryInfoKey(Key, nullptr, nullptr, nullptr, &valueCount, nullptr, NULL, nullptr, nullptr, nullptr, nullptr, nullptr);
		for (DWORD x = 0; x < valueCount; x++)
		{
			WCHAR SubKeyName[MAX_KEY_LENGTH] = { 0 };
			DWORD SubKeyNameLen = MAX_KEY_LENGTH;
			RegEnumKeyEx(Key, x, SubKeyName, &SubKeyNameLen, NULL, NULL, NULL, NULL);
			WCHAR ComponentId[MAX_VALUE_NAME] = { 0 };
			DWORD ComponentIdLen = MAX_VALUE_NAME;
			RegGetValue(Key, SubKeyName, L"ComponentId", RRF_RT_REG_SZ, NULL, &ComponentId, &ComponentIdLen);

			std::wstring TargetName(L"tap0901");
			if (TargetName.compare(ComponentId) == 0)
			{
				WCHAR NetCfgInstanceId[MAX_VALUE_NAME] = { 0 };
				DWORD NetCfgInstanceIdLen = MAX_VALUE_NAME;
				RegGetValue(Key, SubKeyName, L"NetCfgInstanceId", RRF_RT_REG_SZ, NULL, &NetCfgInstanceId, &NetCfgInstanceIdLen);
				std::wstring NICKeyName(L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\");
				NICKeyName.append(NetCfgInstanceId);
				HKEY NICKey;
				RegOpenKeyW(HKEY_LOCAL_MACHINE, NICKeyName.c_str(), &NICKey);
				WCHAR NICName[MAX_VALUE_NAME] = { 0 };
				DWORD NICNameLen = MAX_VALUE_NAME;
				RegGetValue(NICKey, L"Connection", L"Name", RRF_RT_REG_SZ, NULL, &NICName, &NICNameLen);

				struct NIC Interface;
				Interface.ID = NetCfgInstanceId;
				Interface.Name = NICName;
				Me->NICList.push_back(Interface);

				std::wstring Label(Interface.Name);
				Label.append(L" - ");
				Label.append(Interface.ID);
				int pos = SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)Label.c_str());
				SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)Me->NICList.size() - 1);
			}
		}
		RegCloseKey(Key);
	}
	return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_LIST_NICS)
		{
			HWND hwndList = GetDlgItem(hDlg, IDC_LIST_NICS);
			int i = SendMessage(hwndList, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			EnableWindow(GetDlgItem(hDlg, IDOK), i > -1);
		}
		if (LOWORD(wParam) == IDOK)
		{
			HWND hwndList = GetDlgItem(hDlg, IDC_LIST_NICS);
			int i = -1;
			i = SendMessage(hwndList, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			if (i == -1)
			{
				MessageBox(hDlg, L"You must select a NIC", L"Error", 0);
			}
			else
			{
				NICSelector* Me = (NICSelector*)GetWindowLongPtr(hDlg, DWLP_USER);
				i = SendMessage(hwndList, LB_GETITEMDATA, (WPARAM)i, (LPARAM)0);
				Me->SelectedNIC = Me->NICList[i];
				EndDialog(hDlg, LOWORD(wParam));
			}
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_ADD_NIC)
		{
			SHELLEXECUTEINFO info = { 0 };
			info.cbSize = sizeof(info);
			info.hwnd = hDlg;
			info.lpVerb = L"runas";
			info.lpFile = L"tapinstall.exe";
			info.lpParameters = L"install OemVista.inf tap0901";
			info.fMask = SEE_MASK_NOCLOSEPROCESS;
			bool child = ShellExecuteEx(&info);
			if (child) {
				WaitForSingleObject(info.hProcess, INFINITE);
				CloseHandle(info.hProcess);
			}
			else {
				MessageBox(NULL, L"Error adding NIC. Try doing it manually.", NULL, NULL);
			}
			SendMessage(hDlg, WM_USER + 1, 0, 0);
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			NICSelector* Me = (NICSelector*)GetWindowLongPtr(hDlg, DWLP_USER);
			if (Me->Startup)
				PostQuitMessage(0);
			else
				EndDialog(hDlg, 0);
		}
		break;
	}
	return (INT_PTR)FALSE;
}
