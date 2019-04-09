// MegaLAN.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <algorithm>

#include "MegaLAN.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;
HWND hWnd;
NOTIFYICONDATA trayIcon;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LPCWSTR szWindowClass = L"MegaLAN";

NICSelector NICs;
UDPSocket Socket;
Login Session;
Client* VPNClient = NULL;
std::vector<struct sockaddr_in6> MyExternalAddresses;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	HMODULE hModule = GetModuleHandle(NULL);
	WCHAR path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	for (int x = MAX_PATH - 1; x; x--)
	{
		if (path[x] == L'\\')
		{
			path[x] = 0;
			break;
		}
	}
	SetCurrentDirectory(path);

	printf("Statup\n");
	// Initialize global strings
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance))
    {
        return FALSE;
    }

    MSG msg;
	if (lstrcmpW(lpCmdLine, L"/Autorun") == 0)
	{
		printf("Autorun\n");
		const LPCWSTR Software = L"Software\\MegaLAN";
		HKEY Key;
		RegOpenKey(HKEY_CURRENT_USER, Software, &Key);
		WCHAR VLAN[255];
		memset(VLAN, 0, sizeof(VLAN));
		DWORD VLANLen = 254;
		if (RegGetValue(Key, NULL, L"VLAN", RRF_RT_REG_SZ, NULL, VLAN, &VLANLen) != ERROR_SUCCESS)
			VLANLen = 0;
		WCHAR VLANKey[255];
		memset(VLANKey, 0, sizeof(VLANKey));
		DWORD VLANKeyLen = 254;
		if (RegGetValue(Key, NULL, L"VLANKey", RRF_RT_REG_SZ, NULL, VLANKey, &VLANKeyLen) != ERROR_SUCCESS)
			VLANKeyLen = 0;
		BYTE KeyBytes[32];
		swscanf_s(VLANKey, L"%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
			&KeyBytes[0], &KeyBytes[1], &KeyBytes[2], &KeyBytes[3], &KeyBytes[4], &KeyBytes[5], &KeyBytes[6], &KeyBytes[7], &KeyBytes[8], &KeyBytes[9],
			&KeyBytes[10], &KeyBytes[11], &KeyBytes[12], &KeyBytes[13], &KeyBytes[14], &KeyBytes[15], &KeyBytes[16], &KeyBytes[17], &KeyBytes[18], &KeyBytes[19],
			&KeyBytes[20], &KeyBytes[21], &KeyBytes[22], &KeyBytes[23], &KeyBytes[24], &KeyBytes[25], &KeyBytes[26], &KeyBytes[27], &KeyBytes[28], &KeyBytes[29],
			&KeyBytes[30], &KeyBytes[31]);
		WCHAR VLANID[255];
		memset(VLANID, 0, sizeof(VLANID));
		DWORD VLANIDLen = 254;
		if (RegGetValue(Key, NULL, L"VLANID", RRF_RT_REG_SZ, NULL, VLANID, &VLANIDLen) != ERROR_SUCCESS)
			VLANIDLen = 0;
		BYTE IDBytes[20];
		swscanf_s(VLANID, L"%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
			&IDBytes[0], &IDBytes[1], &IDBytes[2], &IDBytes[3], &IDBytes[4], &IDBytes[5], &IDBytes[6], &IDBytes[7], &IDBytes[8], &IDBytes[9],
			&IDBytes[10], &IDBytes[11], &IDBytes[12], &IDBytes[13], &IDBytes[14], &IDBytes[15], &IDBytes[16], &IDBytes[17], &IDBytes[18], &IDBytes[19]);
		if (VLANLen && VLANIDLen && VLANKeyLen)
		{
			printf("Auto join\n");
			VPNClient = new Client(IDBytes, VLAN);
			VPNClient->Login(KeyBytes);
		}
	}

	// Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
	if (VPNClient)
		delete VPNClient;
    return (int) msg.wParam;
}
// Debug version
int main(int argc, char*argv[])
{
	wWinMain(GetModuleHandle(NULL), NULL, (LPWSTR)L"/Autorun", 0);
}
//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = { 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MEGALAN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MEGALAN));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance)
{
	hInst = hInstance; // Store instance handle in our global variable
	hWnd = CreateWindow(szWindowClass, L"MegaLAN", WS_CAPTION| WS_SYSMENU, CW_USEDEFAULT, 0, 600, 500, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd)
	{
		return FALSE;
	}
	NICs.Run();
	Socket.Start();
	Session.Show();

	SetWindowPos(hWnd, NULL, GetSystemMetrics(SM_CXSCREEN) / 2 - 300, GetSystemMetrics(SM_CYSCREEN) / 2 - 240, 600, 480, 0);
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	uint32_t Start = 0;
	PostMessage(hWnd, WM_USER + 10, 0, 0);

	memset(&trayIcon, 0, sizeof(trayIcon));
	trayIcon.cbSize = sizeof(trayIcon);
	trayIcon.hWnd = hWnd;
	wcscpy_s(trayIcon.szTip, L"MegaLAN");
	trayIcon.uCallbackMessage = WM_USER+IDI_MEGALAN;
	trayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	trayIcon.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MEGALAN));
	trayIcon.uID = (UINT)GetTickCount64();
	return TRUE;
}

uint32_t ListLoadPoint;
struct VLAN {
	BYTE ID[20];
	bool IsPassworded;
	std::wstring Name;
	std::wstring Description;
};
std::vector<struct VLAN> VLANs;
int SortCol = 0;
bool SortReverse = false;
bool SortName(struct VLAN &a, struct VLAN &b)
{
	if (SortReverse)
		return _wcsicmp(a.Name.c_str(), b.Name.c_str())>0;
	else
		return _wcsicmp(b.Name.c_str(), a.Name.c_str())>0;
}
bool SortDescription(struct VLAN &a, struct VLAN &b)
{
	if (SortReverse)
		return _wcsicmp(a.Description.c_str(), b.Description.c_str()) > 0;
	else
		return _wcsicmp(b.Description.c_str(), a.Description.c_str()) > 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		CreateWindowEx(0, WC_BUTTON, L"Select NIC", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, rect.left, rect.top, 100, 30, hWnd, (HMENU)1, (HINSTANCE)GetModuleHandle(NULL), 0);
		CreateWindowEx(0, WC_BUTTON, L"Log Out", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, rect.right - 140, rect.top, 70, 30, hWnd, (HMENU)2, (HINSTANCE)GetModuleHandle(NULL), 0);
		CreateWindowEx(0, WC_BUTTON, L"Exit", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, rect.right - 70, rect.top, 70, 30, hWnd, (HMENU)0xFF, (HINSTANCE)GetModuleHandle(NULL), 0);
		CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOVSCROLL, rect.left, rect.top + 30, rect.right - 120, 25, hWnd, (HMENU)0x10, (HINSTANCE)GetModuleHandle(NULL), 0);
		CreateWindowEx(0, WC_BUTTON, L"Join", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, rect.right-120, rect.top + 30, 50, 25, hWnd, (HMENU)0x11, (HINSTANCE)GetModuleHandle(NULL), 0);
		CreateWindowEx(0, WC_BUTTON, L"Refresh", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, rect.right - 70, rect.top + 30, 70, 25, hWnd, (HMENU)0x12, (HINSTANCE)GetModuleHandle(NULL), 0);
		HWND LV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL, rect.left, rect.top+55, rect.right, rect.bottom-55, hWnd, (HMENU)0x20, (HINSTANCE)GetModuleHandle(NULL), 0);
		SendMessage(LV, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
		LVCOLUMN Name;
		Name.mask = LVCF_WIDTH | LVCF_TEXT;
		Name.pszText = (LPWSTR)L"Name";
		Name.cx = 200;
		ListView_InsertColumn(LV, 0, &Name);
		LVCOLUMN Password;
		Password.mask = LVCF_WIDTH | LVCF_TEXT;
		Password.pszText = (LPWSTR)L"";
		Password.cx = 20;
		Password.iSubItem = 1;
		ListView_InsertColumn(LV, 1, &Password);
		LVCOLUMN Description;
		Description.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		Description.pszText = (LPWSTR)L"Description";
		Description.cx = 400;
		Description.iSubItem = 2;
		ListView_InsertColumn(LV, 2, &Description);
	}break;
	case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
			switch (wmId)
			{
			case 1:
			{
				ShowWindow(hWnd, SW_HIDE);
				NICs.Run();
				ShowWindow(hWnd, SW_SHOW);
			}
			break;
			case 2:
			{
				ShowWindow(hWnd, SW_HIDE);
				Session.Show();
				ShowWindow(hWnd, SW_SHOW);
			}
			break;
			case 0x11:
			{
				WCHAR NameRAW[256] = { 0 };
				GetWindowText(GetDlgItem(hWnd, 0x10), NameRAW, 255);
				std::wstring Name = NameRAW;
				std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
				std::string NameUTF8 = utf8_conv.to_bytes(Name);
				Socket.SendToServer((char*)"JOIN", (BYTE*)NameUTF8.c_str(), (int)NameUTF8.size());
			}
			break;
			case 0x12:
				PostMessage(hWnd, WM_USER + 10, 0, 0);
			break;
			case 0xFF:
			{
				if (MessageBox(hWnd, L"Are you sure you want to exit?", L"Exit?", MB_YESNO) == IDYES)
					SendMessage(hWnd, WM_DESTROY, 0, 0);
			}
			break;
			default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_USER + 100:
		Socket.SocketEventMessage();
		break;
	case WM_USER + 0xFF:
		Socket.LANSocketEventMessage();
		break;
	case WM_TIMER:
		Socket.SendToServer((char*)"LIST", (BYTE*)&ListLoadPoint, 4);
		break;
	case WM_USER + 10:
		ListLoadPoint = 0;
		VLANs.clear();
		Socket.SendToServer((char*)"LIST", (BYTE*)&ListLoadPoint, 4);
		ListView_SetItemCountEx(GetDlgItem(hWnd, 0x20), VLANs.size(), LVSICF_NOSCROLL);
		SetTimer(hWnd, 1, 1000, NULL);
		break;
	case WM_NOTIFY:
	{
		LPNMHDR lpnmh = (LPNMHDR)lParam;
		if (lpnmh->code == LVN_GETDISPINFO)
		{
			NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lParam;
			if (plvdi->item.iItem >= VLANs.size())
				return FALSE;
			switch (plvdi->item.iSubItem)
			{
			case 0:
				// rgPetInfo is an array of PETINFO structures.
				plvdi->item.pszText = (LPWSTR)VLANs[plvdi->item.iItem].Name.c_str();
				break;
			case 1:
				if (VLANs[plvdi->item.iItem].IsPassworded)
					plvdi->item.pszText = (LPWSTR)L"P";
				break;
			case 2:
				plvdi->item.pszText = (LPWSTR)VLANs[plvdi->item.iItem].Description.c_str();
				break;
			default:
				break;
			}
			return TRUE;
		}
		if (lpnmh->code == LVN_COLUMNCLICK)
		{
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
			if (pnmv->iSubItem == SortCol)
				SortReverse = !SortReverse;
			printf("Sort by %u %u\n", pnmv->iSubItem, SortReverse);
			SortCol = pnmv->iSubItem;
			SendMessage(hWnd, WM_USER + 20, 0, 0);
		}
		if (lpnmh->code == LVN_ITEMCHANGED)
		{
			int Selected = ListView_GetNextItem(GetDlgItem(hWnd, 0x20), -1, LVNI_SELECTED);
			if (Selected >= 0 && Selected < VLANs.size())
				SetWindowText(GetDlgItem(hWnd, 0x10), VLANs[Selected].Name.c_str());
			printf("Activate item %d\n", Selected);
		}
	}
	break;
	case WM_USER+20:
		if (SortCol == 0)
			std::sort(VLANs.begin(), VLANs.end(), SortName);
		if (SortCol == 2)
			std::sort(VLANs.begin(), VLANs.end(), SortDescription);
		ListView_RedrawItems(GetDlgItem(hWnd, 0x20), 0, VLANs.size());
		break;
	case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
			std::wstring NICStatus(L"Using Interface:\t");
			NICStatus.append(NICs.SelectedNIC.Name);
			RECT rect;
			GetClientRect(hWnd, &rect);
			rect.top += 6;
			rect.left += 105;
			rect.right -= 140;
			rect.bottom = rect.top + 26;
			DrawText(hdc, NICStatus.c_str(), -1, &rect, DT_TOP | DT_LEFT | DT_EXPANDTABS);
            EndPaint(hWnd, &ps);
        }
        break;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_SHOW);
		PostMessage(hWnd, WM_COMMAND, 0xFF, 0);
		return false;
    case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &trayIcon);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void RecvPacket(struct InboundUDP &Packet)
{
	if (Packet.len>=6 && memcmp(Packet.buffer, "AUTHOK", 6) == 0)
	{
		struct sockaddr_in6 Address;
		Address.sin6_family = AF_INET6;
		memcpy(&Address.sin6_addr, Packet.buffer + 6, 16);
		memcpy(&Address.sin6_port, Packet.buffer + 6 + 16, 2);
		char IP[128];
		inet_ntop(AF_INET6, &Address.sin6_addr, IP, sizeof(IP));
		printf("IP: %s Port %u\n", IP, htons(Address.sin6_port));
		for (struct sockaddr_in6 &a : MyExternalAddresses)
		{
			if (memcmp(&a.sin6_addr, &Address.sin6_addr, sizeof(Address.sin6_addr)) == 0 && a.sin6_port == Address.sin6_port)
			{
				return;
			}
		}
		MyExternalAddresses.push_back(Address);
		printf("Added External IP\n");
	}
	else if (Packet.len > 4 && memcmp(Packet.buffer, "LIST", 4) == 0)
	{
		KillTimer(hWnd, 1);
		BYTE count = Packet.buffer[4];
		for (int x = 0; x < count; x++)
		{
			BYTE ID[20];
			memcpy(ID, Packet.buffer + 5 + (x * 20), 20);
			Socket.SendToServer((char*)"INFO", ID, 20);
		}
		if (count)
		{
			ListLoadPoint = *(int*)(Packet.buffer + 5 + (count * 20));
			Socket.SendToServer((char*)"LIST", (BYTE*)&ListLoadPoint, 4);
			SetTimer(hWnd, 1, 1000, NULL);
		}
	}
	else if (Packet.len > 24 && memcmp(Packet.buffer, "INFO", 4) == 0)
	{
		struct VLAN Network = { 0 };
		memcpy(Network.ID, Packet.buffer + 4, 20);
		std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
		Network.Name = utf8_conv.from_bytes((char*)Packet.buffer + 24);
		Network.Description = utf8_conv.from_bytes((char*)Packet.buffer + 25 + strlen((char*)Packet.buffer + 24));
		Network.IsPassworded = Packet.buffer[26 + strlen((char*)Packet.buffer + 24) + strlen((char*)Packet.buffer + 25 + strlen((char*)Packet.buffer + 24))] == 0;
		for (struct VLAN &N : VLANs)
			if (memcmp(N.ID, Network.ID, sizeof(Network.ID)) == 0)
				return;
		VLANs.push_back(Network);
		ListView_SetItemCountEx(GetDlgItem(hWnd, 0x20), VLANs.size(), LVSICF_NOSCROLL);
		printf("List size: %zu\n", VLANs.size());
		SendMessage(hWnd, WM_USER + 20, 0, 0);
	}
	else if (Packet.len > 4 && memcmp(Packet.buffer, "EROR", 4) == 0)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
		std::wstring Error = utf8_conv.from_bytes((char*)Packet.buffer + 4);
		MessageBox(hWnd, Error.c_str(), L"Error", MB_ICONERROR);
	}
	else if (Packet.len > 24 && memcmp(Packet.buffer, "JOIN", 4) == 0)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
		std::wstring Name = utf8_conv.from_bytes((char*)Packet.buffer + 25);
		if (VPNClient)
			delete VPNClient;
		VPNClient = new Client(Packet.buffer + 4, Name);
		VPNClient->Login(Packet.buffer[24] == 1);
	}
}