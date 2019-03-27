#include "stdafx.h"
#include "Crypto.h"
#include "MegaLAN.h"

Client::Client(BYTE* ID, std::wstring Name)
{
	this->Name = Name;
	memcpy(this->ID, ID, 20);
	memset(this->Key, 0, sizeof(this->Key));
}

Client::~Client()
{
	if (Thread != INVALID_HANDLE_VALUE)
		TerminateThread(Thread, 0);
	if (Device != INVALID_HANDLE_VALUE)
		CloseHandle(Device);
	if (hDlg)
	{
		SendMessage(hDlg, WM_CLOSE, 0, 0);
	}
	if (hWnd)
	{
		SendMessage(hWnd, WM_CLOSE, 0, 0);
	}
}

void Client::Login(bool Password)
{
	if (Password)
	{
		DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_PASSWORD), ::hWnd, Client::PasswordWndProc, (LPARAM)this);
	}
	else
		Start();
}
void Client::Login(BYTE* Key)
{
	memcpy(this->Key, Key, 32);
	Start();
}

INT_PTR CALLBACK Client::PasswordWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		Client* SetMe = (Client*)lParam;
		if (SetMe)
		{
			SetWindowLongPtr(hDlg, DWLP_USER, lParam);
			SetMe->hDlg = hDlg;
			SetWindowText(hDlg, SetMe->Name.c_str());
		}
	}
	break;
	case WM_COMMAND:
	{
		if (wParam == IDCANCEL)
			PostMessage(hDlg, WM_CLOSE, 0, 0);
		if (wParam == IDOK)
		{
			Client* Me = (Client*)GetWindowLongPtr(hDlg, DWLP_USER);
			WCHAR Pass[1024];
			GetWindowText(GetDlgItem(hDlg, IDC_PASSWORD), Pass, sizeof(Pass) / 2);
			std::wstring Password = Pass;
			std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
			std::string PasswordUTF8 = utf8_conv.to_bytes(Password);
			BYTE* SHA256 = Crypt.SHA256(PasswordUTF8);
			memcpy(Me->Key, SHA256, 32);
			Me->Start();
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		}
	}
	break;
	case WM_CLOSE:
		Client* Me = (Client*)GetWindowLongPtr(hDlg, DWLP_USER);
		if (Me)
			Me->hDlg = NULL;
		EndDialog(hDlg, 0);
	}
	return FALSE;
}

unsigned int CTL_CODE(unsigned int DeviceType, unsigned int Function, unsigned int Method, unsigned int Access)
{
	return ((DeviceType << 16) | (Access << 14) | (Function << 2) | Method);
}
unsigned int TAP_CONTROL_CODE(unsigned int request, unsigned int method)
{
	return CTL_CODE(0x00000022, request, method, 0);
}

void Client::Start()
{
	PIP_ADAPTER_ADDRESSES pAddressIP = (PIP_ADAPTER_ADDRESSES)malloc(sizeof(IP_ADAPTER_ADDRESSES));;
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_ADDRESSES);
	// Get required buffer size
	if (pAddressIP && GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddressIP, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAddressIP);
		pAddressIP = (PIP_ADAPTER_ADDRESSES)malloc(ulOutBufLen);
	}
	if (pAddressIP == NULL) {
		MessageBox(hDlg, L"Error allocating memory needed to get adapter IP Addresses.\n", L"Error", MB_ICONERROR);
		return;
	}
	if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddressIP, &ulOutBufLen) == NO_ERROR)
	{
		PIP_ADAPTER_ADDRESSES IP = pAddressIP;
		while (IP)
		{
			WCHAR Name[256];
			MultiByteToWideChar(CP_THREAD_ACP, 0, IP->AdapterName, -1, Name, 256);
			if (NICs.SelectedNIC.ID.compare(Name) == 0)
			{
				printf("Name: %s\n", IP->AdapterName);
				printf("Index: %d\n", IP->IfIndex);
				printf("MAC: %02X%02X%02X%02X%02X%02X\n", IP->PhysicalAddress[0], IP->PhysicalAddress[1], IP->PhysicalAddress[2], IP->PhysicalAddress[3],
					IP->PhysicalAddress[4], IP->PhysicalAddress[5]);
				memcpy(MyMAC, IP->PhysicalAddress, 6);
				PIP_ADAPTER_UNICAST_ADDRESS Addr = IP->FirstUnicastAddress;
				while (Addr)
				{
					if (Addr->Address.lpSockaddr->sa_family == AF_INET)
						memcpy(&IPv4_Address, (&((struct sockaddr_in*)Addr->Address.lpSockaddr)->sin_addr), 4);
					if (Addr->Address.lpSockaddr->sa_family == AF_INET6)
						memcpy(&IPv6_Address, (&((struct sockaddr_in6*)Addr->Address.lpSockaddr)->sin6_addr), 16);
					Addr = Addr->Next;
				}
			}
			IP = IP->Next;
		}
	}
	else
	{
		free(pAddressIP);
		MessageBox(hDlg, L"Error getting adaptor info.\n", L"Error", MB_ICONERROR);
		return;
	}

	free(pAddressIP);
	SendRegister();
}
void Client::SendRegister()
{
	printf("Sending VLAN join request\n");
	BYTE Buffer[4096];
	memcpy(Buffer, ID, 20);
	memcpy(Buffer + 20, "LETMEIN", 7);
	memcpy(Buffer + 20 + 7, MyMAC, 6);
	int Count = MyExternalAddresses.size();
	if (Count > 10)
		Count = 10;
	Buffer[20+7+6] = Count;
	for (int x = 0; x < Count; x++)
	{
		memcpy(Buffer + 20 + 7 + 6 + 1 + (18 * x), &MyExternalAddresses[x].sin6_addr, 16);
		memcpy(Buffer + 20 + 7 + 6 + 1 + (18 * x) + 16, &MyExternalAddresses[x].sin6_port, 2);
	}
	int NewLen = Crypt.AES256_Encrypt(Buffer + 20, 7 + 6 + 1 + (18 * Count), Key);
	Socket.SendToServer((char*)"RGST", Buffer, NewLen + 20);
}

void Client::OpenDevice()
{
	if (Device != INVALID_HANDLE_VALUE)
		return;
	std::wstring DeviceFilename = std::wstring(L"\\\\.\\Global\\");
	DeviceFilename += NICs.SelectedNIC.ID;
	DeviceFilename += L".tap";
	Device = CreateFile(DeviceFilename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, NULL);
	if (Device == INVALID_HANDLE_VALUE)
	{
		LPWSTR messageBuffer = nullptr;
		size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);
		MessageBox(hDlg, messageBuffer, L"Error opening device.", MB_ICONERROR);
		LocalFree(messageBuffer);
		return;
	}
	DWORD Ret;
	DWORD Status = 1;
	DeviceIoControl(Device, TAP_CONTROL_CODE(6, 0), &Status, sizeof(Status), &Status, sizeof(Status), &Ret, NULL);
	if (!(Ret == Status == 1))
	{
		MessageBox(hDlg, L"There was an error connecting the virtual cable.", L"Error opening device.", MB_ICONERROR);
		return;
	}
	Thread = CreateThread(NULL, 1024 * 1024, Client::ThreadProc, (LPVOID)this, 0, NULL);
}

INT_PTR CALLBACK Client::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		ListView_SetExtendedListViewStyle(GetDlgItem(hWnd, IDC_PEERS), LVS_EX_FULLROWSELECT);
		RECT rect;
		GetWindowRect(GetDlgItem(hWnd, IDC_PEERS), &rect);
		LVCOLUMN PeerCol;
		PeerCol.mask = LVCF_WIDTH | LVCF_TEXT;
		PeerCol.pszText = (LPWSTR)L"Name";
		PeerCol.cx = rect.right - rect.left - 160;
		ListView_InsertColumn(GetDlgItem(hWnd, IDC_PEERS), 0, &PeerCol);
		PeerCol.pszText = (LPWSTR)L"";
		PeerCol.cx = 50;
		ListView_InsertColumn(GetDlgItem(hWnd, IDC_PEERS), 1, &PeerCol);
		PeerCol.pszText = (LPWSTR)L"MAC";
		PeerCol.cx = 110;
		ListView_InsertColumn(GetDlgItem(hWnd, IDC_PEERS), 2, &PeerCol);

		PeerCol.pszText = (LPWSTR)L"";
		PeerCol.cx = 30;
		ListView_InsertColumn(GetDlgItem(hWnd, IDC_PEERADDR), 0, &PeerCol);
		PeerCol.pszText = (LPWSTR)L"IP";
		PeerCol.cx = 110;
		ListView_InsertColumn(GetDlgItem(hWnd, IDC_PEERADDR), 1, &PeerCol);
		PeerCol.pszText = (LPWSTR)L"Port";
		PeerCol.cx = 50;
		ListView_InsertColumn(GetDlgItem(hWnd, IDC_PEERADDR), 2, &PeerCol);
		PeerCol.pszText = (LPWSTR)L"Latency";
		PeerCol.cx = 50;
		ListView_InsertColumn(GetDlgItem(hWnd, IDC_PEERADDR), 3, &PeerCol);
		PeerCol.pszText = (LPWSTR)L"Flags";
		PeerCol.cx = 50;
		ListView_InsertColumn(GetDlgItem(hWnd, IDC_PEERADDR), 4, &PeerCol);
		PeerCol.pszText = (LPWSTR)L"Check";
		PeerCol.cx = 70;
		ListView_InsertColumn(GetDlgItem(hWnd, IDC_PEERADDR), 5, &PeerCol);
		PeerCol.pszText = (LPWSTR)L"Recv";
		PeerCol.cx = 70;
		ListView_InsertColumn(GetDlgItem(hWnd, IDC_PEERADDR), 6, &PeerCol);

		HIMAGELIST IMGLST = ImageList_Create(1, 50, 0, 0, 1);
		ListView_SetImageList(GetDlgItem(hWnd, IDC_PEERS), IMGLST, LVSIL_NORMAL);
		Client* SetMe = (Client*)lParam;
		if (!SetMe)
		{
			EndDialog(hWnd, 0);
			return FALSE;
		}
		ListView_SetItemCountEx(GetDlgItem(hWnd, IDC_PEERS), SetMe->PeerList.size(), LVSICF_NOSCROLL);
		SetWindowLongPtr(hWnd, DWLP_USER, lParam);
		SetMe->hWnd = hWnd;
		SetWindowText(hWnd, SetMe->Name.c_str());
		CHAR IPString[1024];
		char IPv4[128];
		inet_ntop(AF_INET, &SetMe->IPv4_Address, IPv4, sizeof(IPv4));
		char IPv6[128];
		inet_ntop(AF_INET6, &SetMe->IPv6_Address, IPv6, sizeof(IPv6));
		sprintf_s(IPString, sizeof(IPString), "IPv4: %s\nIPv6: %s\nMAC: %02X:%02X:%02X:%02X:%02X:%02X\n", IPv4, IPv6,
			SetMe->MyMAC[0], SetMe->MyMAC[1], SetMe->MyMAC[2], SetMe->MyMAC[3], SetMe->MyMAC[4], SetMe->MyMAC[5]);
		for (auto &A : MyExternalAddresses)
		{
			inet_ntop(AF_INET6, &A.sin6_addr, IPv6, sizeof(IPv6));
			sprintf_s(IPString + strlen(IPString), sizeof(IPString) - strlen(IPString), "IP: %s (%u)\n", IPv6, htons(A.sin6_port));
		}
		SetWindowTextA(GetDlgItem(hWnd, IDC_IPv4), IPString);

		trayIcon.hWnd = hWnd;
		ShowWindow(::hWnd, SW_HIDE);
		Shell_NotifyIcon(NIM_ADD, &trayIcon);
		SetTimer(hWnd, 0, 60000, NULL);
		SetTimer(hWnd, 1, 1000, NULL);
		SetTimer(hWnd, 2, 10000, NULL);
		PostMessage(hWnd, WM_TIMER, 2, 0);

		const LPCWSTR Software = L"Software";
		HKEY Key, MyKey;
		RegOpenKey(HKEY_CURRENT_USER, Software, &Key);
		RegCreateKeyEx(Key, L"MegaLAN", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &MyKey, NULL);
		RegSetKeyValue(MyKey, NULL, L"VLAN", REG_SZ, SetMe->Name.c_str(), SetMe->Name.length() * 2 + 2);
		WCHAR IDStr[100];
		for (int x = 0; x < 20; x++)
			wsprintf(IDStr + x * 2, L"%02X ", SetMe->ID[x]);
		RegSetKeyValue(MyKey, NULL, L"VLANID", REG_SZ, IDStr, lstrlenW(IDStr) * 2 + 2);
		WCHAR KeyStr[100];
		for (int x = 0; x < 32; x++)
			wsprintf(KeyStr + x * 2, L"%02X ", SetMe->Key[x]);
		RegSetKeyValue(MyKey, NULL, L"VLANKey", REG_SZ, KeyStr, lstrlenW(KeyStr) * 2 + 2);
		RegCloseKey(MyKey);
		RegCloseKey(Key);

		HMODULE hModule = GetModuleHandle(NULL);
		WCHAR Command[MAX_PATH+20];
		GetModuleFileName(hModule, Command, MAX_PATH);
		lstrcpyW(Command + lstrlenW(Command), L" /Autorun");
		const LPCWSTR AutoRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
		Key = 0;
		RegOpenKey(HKEY_CURRENT_USER, AutoRunKey, &Key);
		RegSetKeyValue(Key, NULL, L"MegaLAN", REG_SZ, Command, lstrlenW(Command) * 2 + 2);
		RegCloseKey(Key);
	}
	break;
	case WM_NOTIFY:
	{
		LPNMHDR lpnmh = (LPNMHDR)lParam;
		if (lpnmh->code == LVN_GETDISPINFO)
		{
			Client* Me = (Client*)GetWindowLongPtr(hWnd, DWLP_USER);
			if (!Me) return FALSE;
			NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lParam;
			if (lpnmh->hwndFrom == GetDlgItem(hWnd, IDC_PEERS))
			{
				if (plvdi->item.iItem >= Me->PeerList.size())
					return FALSE;
				switch (plvdi->item.iSubItem)
				{
				case 0:
					plvdi->item.pszText = (LPWSTR)Me->PeerList[plvdi->item.iItem].Name.c_str();
					break;
				case 1:
					if (Me->PeerList[plvdi->item.iItem].PreferredAddress)
					{
						WCHAR Latency[10];
						wsprintf(Latency, L"%ums", Me->PeerList[plvdi->item.iItem].PreferredAddress->Latency);
						plvdi->item.pszText = Latency;
					}
					else
					{
						plvdi->item.pszText = (LPWSTR)L"!!!!!";
					}
					break;
				case 2:
					WCHAR MAC[20];
					wsprintf(MAC, L"%02X:%02X:%02X:%02X:%02X:%02X",
						Me->PeerList[plvdi->item.iItem].MAC[0], Me->PeerList[plvdi->item.iItem].MAC[1], Me->PeerList[plvdi->item.iItem].MAC[2],
						Me->PeerList[plvdi->item.iItem].MAC[3], Me->PeerList[plvdi->item.iItem].MAC[4], Me->PeerList[plvdi->item.iItem].MAC[5]);
					plvdi->item.pszText = MAC;
					break;
				}
				return TRUE;
			}
			if (lpnmh->hwndFrom == GetDlgItem(hWnd, IDC_PEERADDR))
			{
				int Selected = ListView_GetNextItem(GetDlgItem(hWnd, IDC_PEERS), -1, LVNI_SELECTED);
				if (Selected >= 0)
				{
					std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
					switch (plvdi->item.iSubItem)
					{
					case 0:
					{
						static WCHAR State[10];
						if (Me->PeerList[Selected].PreferredAddress == &Me->PeerList[Selected].Addresses[plvdi->item.iItem])
							wsprintf(State, L"%d*", Me->PeerList[Selected].Addresses[plvdi->item.iItem].State);
						else
							wsprintf(State, L"%d", Me->PeerList[Selected].Addresses[plvdi->item.iItem].State);
						plvdi->item.pszText = (LPWSTR)State;
					}
					break;
					case 1:
					{
						plvdi->item.pszText = (LPWSTR)Me->PeerList[Selected].Addresses[plvdi->item.iItem].IPString;
					}
					break;
					case 2:
					{
						static WCHAR Port[10];
						wsprintf(Port, L"%d", htons(Me->PeerList[Selected].Addresses[plvdi->item.iItem].Address.sin6_port));
						plvdi->item.pszText = (LPWSTR)Port;
					}
					break;
					case 3:
					{
						if (Me->PeerList[Selected].Addresses[plvdi->item.iItem].State > 0)
						{
							static WCHAR Latency[10];
							wsprintf(Latency, L"%dms", Me->PeerList[Selected].Addresses[plvdi->item.iItem].Latency);
							plvdi->item.pszText = (LPWSTR)Latency;
						}
						else
							plvdi->item.pszText = (LPWSTR)L"";
					}
					break;
					case 4:
					{
						static std::wstring Flags = L"";
						Flags = L"";
						if (Me->PeerList[Selected].Addresses[plvdi->item.iItem].DiscoveryFlags.FromServer)
							Flags += L"S";
						if (Me->PeerList[Selected].Addresses[plvdi->item.iItem].DiscoveryFlags.FromPeer)
							Flags += L"P";
						if (Me->PeerList[Selected].Addresses[plvdi->item.iItem].DiscoveryFlags.FromINIT)
							Flags += L"I";
						if (Me->PeerList[Selected].Addresses[plvdi->item.iItem].DiscoveryFlags.FromLAN)
							Flags += L"L";
						plvdi->item.pszText = (LPWSTR)Flags.c_str();
					}
					break;
					case 5:
					{
						static WCHAR CheckTime[10];
						wsprintf(CheckTime, L"%d", (GetTickCount64() - Me->PeerList[Selected].Addresses[plvdi->item.iItem].LastCheckTime)/1000);
						plvdi->item.pszText = (LPWSTR)CheckTime;
					}
					break;
					case 6:
					{
						if (Me->PeerList[Selected].Addresses[plvdi->item.iItem].LastRecvTime)
						{
							static WCHAR RecvTime[10];
							wsprintf(RecvTime, L"%d", (GetTickCount64() - Me->PeerList[Selected].Addresses[plvdi->item.iItem].LastRecvTime)/1000);
							plvdi->item.pszText = (LPWSTR)RecvTime;
						}
						else
							plvdi->item.pszText = (LPWSTR)L"";
					}
					break;
					}
					return TRUE;
				}
			}
		}
		if (lpnmh->code == LVN_ITEMCHANGED)
		{
			Client* Me = (Client*)GetWindowLongPtr(hWnd, DWLP_USER);
			if (!Me) return FALSE;
			int Selected = ListView_GetNextItem(GetDlgItem(hWnd, IDC_PEERS), -1, LVNI_SELECTED);
			WCHAR PeerInfo[1024] = { 0 };
			if (Selected >= 0)
			{
				wsprintf(PeerInfo, L"%s (%02X:%02X:%02X:%02X:%02X:%02X)", Me->PeerList[Selected].Name.c_str(),
					Me->PeerList[Selected].MAC[0], Me->PeerList[Selected].MAC[1], Me->PeerList[Selected].MAC[2],
					Me->PeerList[Selected].MAC[3], Me->PeerList[Selected].MAC[4], Me->PeerList[Selected].MAC[5]);
				SetWindowText(GetDlgItem(hWnd, IDC_SELECTED_PEER), PeerInfo);
				PostMessage(hWnd, WM_TIMER, 1, 0);
			}
			else
				ListView_SetItemCount(GetDlgItem(hWnd, IDC_PEERADDR), 0);
			ListView_Update(GetDlgItem(hWnd, IDC_PEERADDR), 0);
		}
	}
	break;
	case WM_USER + IDI_MEGALAN:
		if (lParam == WM_LBUTTONDOWN)
		{
			ShowWindow(hWnd, SW_SHOW);
			SetForegroundWindow(hWnd);
		}
		break;
	case WM_USER + 1:
	{
		Client* Me = (Client*)GetWindowLongPtr(hWnd, DWLP_USER);
		if (!Me) return FALSE;
		Me->ReadPacket((BYTE*)lParam, wParam);
	}
	break;
	case WM_TIMER:
	{
		if (wParam == 0)
		{
			Client* Me = (Client*)GetWindowLongPtr(hWnd, DWLP_USER);
			if (!Me) return FALSE;
			Me->SendRegister();
		}
		if (wParam == 1)
		{
			Client* Me = (Client*)GetWindowLongPtr(hWnd, DWLP_USER);
			if (!Me) return FALSE;
			for (auto it = Me->PeerList.begin(); it < Me->PeerList.end(); )
			{
				if (it->Timeout < GetTickCount64())
				{
					Me->PeerList.erase(it);
					it = Me->PeerList.begin();
				}
				else
				{
					it->Poll();
					it++;
				}
			}
			ListView_SetItemCount(GetDlgItem(hWnd, IDC_PEERS), Me->PeerList.size());
			int Selected = ListView_GetNextItem(GetDlgItem(hWnd, IDC_PEERS), -1, LVNI_SELECTED);
			if (Selected >= 0 && Selected < Me->PeerList.size())
				ListView_SetItemCountEx(GetDlgItem(hWnd, IDC_PEERADDR), Me->PeerList[Selected].Addresses.size(), LVSICF_NOSCROLL);
			else
				ListView_SetItemCount(GetDlgItem(hWnd, IDC_PEERADDR), 0);
			ListView_Update(GetDlgItem(hWnd, IDC_PEERADDR), 0);
			ListView_Update(GetDlgItem(hWnd, IDC_PEERS), 0);
		}
		if (wParam == 2)
		{
			SetTimer(hWnd, 2, 30000, NULL);
			Client* Me = (Client*)GetWindowLongPtr(hWnd, DWLP_USER);
			if (!Me) return FALSE;
			PIP_ADAPTER_ADDRESSES pAddressIP = (PIP_ADAPTER_ADDRESSES)malloc(sizeof(IP_ADAPTER_ADDRESSES));;
			ULONG ulOutBufLen = sizeof(IP_ADAPTER_ADDRESSES);
			// Get required buffer size
			if (pAddressIP && GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddressIP, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
			{
				free(pAddressIP);
				pAddressIP = (PIP_ADAPTER_ADDRESSES)malloc(ulOutBufLen);
			}
			if (pAddressIP == NULL) {
				return FALSE;
			}
			if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddressIP, &ulOutBufLen) == NO_ERROR)
			{
				PIP_ADAPTER_ADDRESSES IP = pAddressIP;
				while (IP)
				{
					WCHAR Name[256];
					MultiByteToWideChar(CP_THREAD_ACP, 0, IP->AdapterName, -1, Name, 256);
					if (NICs.SelectedNIC.ID.compare(Name))
					{
						PIP_ADAPTER_UNICAST_ADDRESS Addr = IP->FirstUnicastAddress;
						while (Addr)
						{
							if (Addr->Address.lpSockaddr->sa_family == AF_INET)
							{
								struct in_addr Broadcast = { 0 };
								memcpy(&Broadcast, &((struct sockaddr_in*)Addr->Address.lpSockaddr)->sin_addr, 4);
								*(UINT32*)&Broadcast |= 0xFFFFFFFF << Addr->OnLinkPrefixLength;
								char IP[128];
								inet_ntop(AF_INET, &Broadcast, IP, sizeof(IP));
								if (Addr->PreferredLifetime > 0)
								{
									struct sockaddr_in6 Address = { 0 };
									Address.sin6_family = AF_INET6;
									char IP[128] = { 0 };
									strcpy_s(IP, "::ffff:");
									inet_ntop(AF_INET, &Broadcast, IP+7, sizeof(IP)-7);
									printf("%s\n", IP);
									inet_pton(AF_INET6, IP, &Address.sin6_addr);
									Address.sin6_port = htons(LAN_DISCOVERY_PORT);
									BYTE Buffer[100];
									memcpy(Buffer, "LAND", 4);
									memcpy(Buffer + 4, Me->ID, 20);
									memcpy(Buffer + 24, Socket.AuthID, 20);
									memcpy(Buffer + 44, Me->MyMAC, 6);
									Socket.SendToPeer(Address, Buffer, 50);
								}
							}
							Addr = Addr->Next;
						}
					}
					IP = IP->Next;
				}
			}

			free(pAddressIP);
		}
	}
	break;
	case WM_COMMAND:
	{
		if (wParam == IDC_DISCONNECT)
		{
			KillTimer(hWnd, 0);
			KillTimer(hWnd, 1);
			KillTimer(hWnd, 2);
			EndDialog(hWnd, 0);
			Client* Me = (Client*)GetWindowLongPtr(hWnd, DWLP_USER);
			if (!Me) return FALSE;
			Shell_NotifyIcon(NIM_DELETE, &trayIcon);
			ShowWindow(::hWnd, SW_SHOW);
			Me->hWnd = NULL;
			if (Me->Thread != INVALID_HANDLE_VALUE)
				TerminateThread(Me->Thread, 0);
			if (Me->Device != INVALID_HANDLE_VALUE)
				CloseHandle(Me->Device);
			Me->Thread = INVALID_HANDLE_VALUE;
			Me->Device = INVALID_HANDLE_VALUE;
			memset(Me->MyMAC, 0, 6);

			const LPCWSTR RegKey = L"Software\\MegaLAN";
			HKEY Key;
			RegOpenKey(HKEY_CURRENT_USER, RegKey, &Key);
			RegDeleteKeyValue(Key, NULL, L"VLAN");
			RegDeleteKeyValue(Key, NULL, L"VLANID");
			RegDeleteKeyValue(Key, NULL, L"VLANKey");
			RegCloseKey(Key);
			const LPCWSTR AutoRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
			Key = 0;
			RegOpenKey(HKEY_CURRENT_USER, AutoRunKey, &Key);
			RegDeleteKeyValue(Key, NULL, L"MegaLAN");
			RegCloseKey(Key);
		}
	}
	break;
	case WM_CLOSE:
	{
		ShowWindow(hWnd, SW_HIDE);
	}
	break;
	}
	return FALSE;
}

void Client::RecvPacketFromServer(struct InboundUDP &Packet)
{
	if (memcmp(Packet.buffer, "PEER", 4) == 0)
	{
		for (auto &P : PeerList)
		{
			if (memcmp(P.UserID, Packet.buffer + 4, 20) == 0)
			{
				std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
				P.Name = utf8_conv.from_bytes((char*)Packet.buffer + 24);
			}
		}
		return;
	}
	if (memcmp(Packet.buffer + 4, ID, 20) != 0)
	{
		printf("VPNClient packet has wrong VLAN ID\n");
		return;
	}
	if (memcmp(Packet.buffer, "RGST", 4) == 0)
	{
		memcpy(&IPv4_Address, Packet.buffer + 24, 4);
		IPv4_PrefixLength = Packet.buffer[28];
//		memcpy(&IPv6_Address, Packet.buffer + 29, 16);
//		IPv6_PrefixLength = Packet.buffer[45];
		char IPv4[128];
		inet_ntop(AF_INET, &IPv4_Address, IPv4, sizeof(IPv4));
//		char IPv6[128];
//		inet_ntop(AF_INET6, &IPv6_Address, IPv6, sizeof(IPv6));
		if (Device == INVALID_HANDLE_VALUE)
			OpenDevice();
		if (!hWnd)
		{
			CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_CLIENT), ::hWnd, Client::WndProc, (LPARAM)this);
		}
	}
	if (memcmp(Packet.buffer, "VLAN", 4) == 0)
	{
		if (memcmp(ID, Packet.buffer + 4, 20) != 0)
			return;
		if (memcmp(Socket.AuthID, Packet.buffer + 24, 20) == 0 && memcmp(Packet.buffer + 44, MyMAC, 6) == 0)
			return;
		Peer NewPeer(Packet.buffer + 24, Packet.buffer + 44);
		for (auto &P : PeerList)
		{
			if (NewPeer == P)
			{
				P.RegisterAddress(*(struct in_addr6*)(Packet.buffer + 50), htons(*(UINT16*)(Packet.buffer + 66)), DISCOVERY_FROM_SERVER);
				return;
			}
		}
		NewPeer.RegisterAddress(*(struct in_addr6*)(Packet.buffer + 50), htons(*(UINT16*)(Packet.buffer + 66)), DISCOVERY_FROM_SERVER);
		PeerList.push_back(NewPeer);
		ListView_SetItemCountEx(GetDlgItem(hWnd, IDC_PEERS), PeerList.size(), LVSICF_NOSCROLL);
	}
}
void Client::RegisterPeer(BYTE* UserID, BYTE* MAC, struct in6_addr &Address, UINT16 Port, UINT DiscoverySource)
{
	if (memcmp(UserID, Socket.AuthID, 20) == 0 && memcmp(MAC, MyMAC, 6) == 0)
		return;
	Peer NewPeer(UserID, MAC);
	for (auto &P : PeerList)
	{
		if (NewPeer == P)
		{
			P.RegisterAddress(Address, Port, DiscoverySource);
			return;
		}
	}
	NewPeer.RegisterAddress(Address, Port, DiscoverySource);
	PeerList.push_back(NewPeer);
	printf("Register new peer\n");
}
void Client::RecvPacket(struct InboundUDP &Packet)
{
	if (memcmp(Packet.buffer, "ETHN", 4) == 0 && Packet.len > 16)
	{
		OVERLAPPED DeviceOL = { 0 };
		WriteFile(Device, Packet.buffer + 4, Packet.len - 4, NULL, &DeviceOL);
		struct MAC FromMAC;
		memcpy(FromMAC.bytes, Packet.buffer + 10, 6);
		printf("Received packet from ");
		for (int x = 0; x < 6; x++)
		{
			printf("%02X:", FromMAC.bytes[x]);
		}
		printf("\n");
		struct SwitchingTableEntry Entry = { Packet.addr, GetTickCount64() + 10000 };
		SwitchingTable[FromMAC] = Entry;
		return;
	}
	if (memcmp(Packet.buffer, "PING", 4) == 0 || memcmp(Packet.buffer, "INIT", 4) == 0 || memcmp(Packet.buffer, "LAND", 4) == 0)
	{
		if (memcmp(ID, Packet.buffer + 4, 20) != 0)
			return;
		if (memcmp(Socket.AuthID, Packet.buffer + 24, 20) == 0 && memcmp(Packet.buffer + 44, MyMAC, 6) == 0)
			return;

		if (memcmp(Packet.buffer, "PING", 4) == 0)
		{
			BYTE Buffer[4096];
			memcpy(Buffer, "PONG", 4);
			memcpy(Buffer + 4, Socket.AuthID, 20);
			memcpy(Buffer + 24, MyMAC, 6);
			*(UINT16*)(Buffer + 30) = htons(0);
			Socket.SendToPeer(Packet.addr, Buffer, 32);
			RegisterPeer(Packet.buffer + 24, Packet.buffer + 44, Packet.addr.sin6_addr, htons(Packet.addr.sin6_port), DISCOVERY_FROM_INIT);
		}
		else if (memcmp(Packet.buffer, "INIT", 4) == 0 || memcmp(Packet.buffer, "LAND", 4) == 0)
		{
			BYTE Buffer[4096];
			if (memcmp(Packet.buffer, "LAND", 4) == 0)
				memcpy(Buffer, "LANR", 4);
			else
				memcpy(Buffer, "PONG", 4);
			UINT16 Count = 0;
			for (int index = 0; index < PeerList.size(); ++index)
			{
				if (PeerList[index].PreferredAddress && !( memcmp(PeerList[index].UserID, Packet.buffer + 24, 20) == 0 && memcmp(PeerList[index].MAC + 44, MyMAC, 6) == 0 ))
				{
					memcpy(Buffer + 32 + (Count * 44), &PeerList[index].UserID, 20);
					memcpy(Buffer + 32 + (Count * 44) + 20, &PeerList[index].MAC, 6);
					memcpy(Buffer + 32 + (Count * 44) + 26, &PeerList[index].PreferredAddress->Address.sin6_addr, 16);
					memcpy(Buffer + 32 + (Count * 44) + 42, &PeerList[index].PreferredAddress->Address.sin6_port, 2);
					Count++;
				}
			}
			memcpy(Buffer + 4, Socket.AuthID, 20);
			memcpy(Buffer + 24, MyMAC, 6);
			*(UINT16*)(Buffer + 30) = htons(Count);
			Socket.SendToPeer(Packet.addr, Buffer, 32 + (Count * 44));
			if (memcmp(Packet.buffer, "LAND", 4) == 0)
				RegisterPeer(Packet.buffer + 24, Packet.buffer + 44, Packet.addr.sin6_addr, htons(Packet.addr.sin6_port), DISCOVERY_FROM_LAN);
			else
				RegisterPeer(Packet.buffer + 24, Packet.buffer + 44, Packet.addr.sin6_addr, htons(Packet.addr.sin6_port), DISCOVERY_FROM_INIT);
		}
	}
	if (memcmp(Packet.buffer, "PONG", 4) == 0 || memcmp(Packet.buffer, "LANR", 4) == 0)
	{
		if (memcmp(Socket.AuthID, Packet.buffer + 4, 20) == 0 && memcmp(Packet.buffer + 24, MyMAC, 6) == 0)
			return;
		for (auto &P : PeerList)
		{
			if (memcmp(P.UserID, Packet.buffer + 4, 20) == 0 && memcmp(P.MAC, Packet.buffer + 24, 6) == 0)
			{
				if (memcmp(Packet.buffer, "PONG", 4) == 0)
					P.Pong(Packet.addr);
				if (Packet.len >= 32)
				{
					UINT Count = htons(*(UINT16*)(Packet.buffer + 30));
					for (int x = 0; x < Count && (Count * 44 + 6) <= Packet.len; x++)
					{
						if (memcmp(Packet.buffer, "LANR", 4) == 0)
							RegisterPeer(Packet.buffer + 32 + (x * 44), Packet.buffer + 32 + (x * 44) + 20, *(struct in6_addr*)(Packet.buffer + 32 + (x * 44) + 26), htons(*(UINT*)(Packet.buffer + 32 + (x * 44) + 42)), DISCOVERY_FROM_LAN);
						else
							RegisterPeer(Packet.buffer + 32 + (x * 44), Packet.buffer + 32 + (x * 44) + 20, *(struct in6_addr*)(Packet.buffer + 32 + (x * 44) + 26), htons(*(UINT*)(Packet.buffer + 32 + (x * 44) + 42)), DISCOVERY_FROM_PEER);
					}
				}
				return;
			}
		}
	}
}

uint16_t ip_checksum(void* vdata, size_t length) {
	// Cast the data pointer to one that can be indexed.
	char* data = (char*)vdata;

	// Initialise the accumulator.
	uint32_t acc = 0xffff;

	// Handle complete 16-bit blocks.
	for (size_t i = 0; i + 1 < length; i += 2) {
		uint16_t word;
		memcpy(&word, data + i, 2);
		acc += ntohs(word);
		if (acc > 0xffff) {
			acc -= 0xffff;
		}
	}

	// Handle any partial block at the end of the data.
	if (length & 1) {
		uint16_t word = 0;
		memcpy(&word, data + length - 1, 1);
		acc += ntohs(word);
		if (acc > 0xffff) {
			acc -= 0xffff;
		}
	}

	// Return the checksum in network byte order.
	return htons(~acc);
}
struct iphdr {
	uint8_t  verhdrlen;
	uint8_t  service;
	uint16_t len;
	uint16_t ident;
	uint16_t frags;
	uint8_t  ttl;
	uint8_t  protocol;
	uint16_t chksum;
	struct in_addr src;
	struct in_addr dest;
};
struct udphdr {
	uint16_t srcport;
	uint16_t dstport;
	uint16_t length;
	uint16_t chksum;
};
void Client::ReadPacket(BYTE* Buffer, WORD len)
{
	struct MAC ToMAC;
	memcpy(ToMAC.bytes, Buffer, 6);
	BYTE* FromMAC = Buffer+6;
	UINT16 EthernetType = htons(*(UINT16*)(Buffer + 12));
	if (EthernetType == 0x0800 && memcmp(MyMAC, FromMAC, 6) == 0)
	{
		struct iphdr* IPv4Header = (struct iphdr*) (Buffer + 14);
		BYTE IPv4HeaderLength = (IPv4Header->verhdrlen & 0x0F)<<2;
		BYTE Protocol = IPv4Header->protocol;
		struct in_addr &SourceIPv4 = IPv4Header->src;
		if (memcmp(&SourceIPv4, &IPv4_Address, 4) && *(UINT32*)&SourceIPv4 != INADDR_ANY)
		{
			char IP[128];
			inet_ntop(AF_INET, &SourceIPv4, IP, sizeof(IP));
			printf("Spoofed source IPv4 %s\n", IP);
			return;
		}
		if (Protocol == 0x11)
		{
			struct udphdr* UDPHeader = (struct udphdr*)((BYTE*)IPv4Header + IPv4HeaderLength);
			UINT16 UDPSrcPort = htons(UDPHeader->srcport);
			UINT16 UDPDestPort = htons(UDPHeader->dstport);
			if (UDPDestPort == 67)
			{
				BYTE* Payload = (BYTE*)UDPHeader + sizeof(struct udphdr);
				if (Payload[0] == 1) // Boot Request
				{
					DWORD TransactionID = *(DWORD*)(Payload + 4);

					BYTE DHCPResponse[1500] = { 0 };
					memcpy(DHCPResponse, Buffer, 6);// To MAC
					memcpy(DHCPResponse + 6, Buffer + 6, 6);// From MAC
					DHCPResponse[7] ^= 0xFF; // Toggle 1 byte of From MAC
					*(UINT16*)(DHCPResponse + 12) = htons(0x0800);// EtherType

					struct iphdr* ResponseIPHeader = (struct iphdr*)(DHCPResponse + 14);
					ResponseIPHeader->verhdrlen = 0x45;
					ResponseIPHeader->ident = 0xF00D;
					ResponseIPHeader->frags = 0;
					ResponseIPHeader->ttl = 64;
					ResponseIPHeader->protocol = 0x11;
					inet_pton(AF_INET, "10.0.0.0", &ResponseIPHeader->src);
					ResponseIPHeader->dest = IPv4Header->src;
					if (*(UINT32*)&ResponseIPHeader->dest == INADDR_ANY)
						inet_pton(AF_INET, "255.255.255.255", &ResponseIPHeader->dest);

					struct udphdr* ResponseUDPHeader = (struct udphdr*)((BYTE*)ResponseIPHeader + sizeof(struct iphdr));
					ResponseUDPHeader->srcport = htons(67);
					ResponseUDPHeader->dstport = htons(UDPSrcPort);

					BYTE* ResponseDHCPPacket = (BYTE*)ResponseUDPHeader + sizeof(struct udphdr);
					ResponseDHCPPacket[0] = 2; // Boot Reply
					ResponseDHCPPacket[1] = 1; // Type Ethernet
					ResponseDHCPPacket[2] = 6; // HW Address Length
					ResponseDHCPPacket[3] = 0; // Hops
					*(UINT32*)(ResponseDHCPPacket + 4) = TransactionID; // Copy Transaction ID
					if (*(UINT32*)&ResponseIPHeader->dest == INADDR_ANY)
						*(UINT16*)(ResponseDHCPPacket + 10) = htons(0x8000); // broadcast flags
					*(struct in_addr*)(ResponseDHCPPacket + 12) = IPv4Header->src;	// Client IP
					*(struct in_addr*)(ResponseDHCPPacket + 16) = this->IPv4_Address;	// Assigned IP
					inet_pton(AF_INET, "127.0.0.1", (struct in_addr*)(ResponseDHCPPacket + 20)); // Server IP
					inet_pton(AF_INET, "0.0.0.0", (struct in_addr*)(ResponseDHCPPacket + 24)); // Relay Agent
					memcpy(ResponseDHCPPacket + 28, FromMAC, 6); // Client MAC
					
					*(DWORD*)(ResponseDHCPPacket + 236) = htonl(0x63825363); // Magic Cookie
					BYTE* Option = ResponseDHCPPacket + 240;

					Option[0] = 53;	// Message Type (I Assume it is the first option in the request)
					Option[1] = 1;		// Length 1
					if (Payload[242] == 1) // DISCOVER
						Option[2] = 2;	// OFFER
					else // REQUEST/INFORM
						Option[2] = 5;	// ACK
					Option += 3;

					Option[0] = 54;
					Option[1] = 4;
					*(DWORD*)(Option + 2) = *(DWORD*)(ResponseDHCPPacket + 20);
					Option += 6;
					Option[0] = 51;
					Option[1] = 4;
					*(DWORD*)(Option + 2) = htonl(3600);
					Option += 6;
					Option[0] = 58;
					Option[1] = 4;
					*(DWORD*)(Option + 2) = htonl(1800);
					Option += 6;
					Option[0] = 59;
					Option[1] = 4;
					*(DWORD*)(Option + 2) = htonl(2400);
					Option += 6;
					Option[0] = 1;
					Option[1] = 4;
					*(DWORD*)(Option + 2) = htonl(0xFFFFFFFF << (32 - this->IPv4_PrefixLength));
					Option += 6;
					Option[0] = 28;
					Option[1] = 4;
					*(DWORD*)(Option + 2) = htonl(0xFFFFFFFF >> this->IPv4_PrefixLength) | *(DWORD*)&this->IPv4_Address;
					Option += 6;
					Option[0] = 0xFF;
					Option += 1;

					ResponseIPHeader->len = htons(Option - 14 - DHCPResponse);
					ResponseUDPHeader->length = htons(Option - 34 - DHCPResponse);
					ResponseIPHeader->chksum = (UINT16)ip_checksum(ResponseIPHeader, sizeof(struct iphdr));
					OVERLAPPED ol = { 0 };
					WriteFile(Device, DHCPResponse, Option - DHCPResponse, NULL, &ol);
					printf("DHCP Request, answered locally\n");
					return;
				}
			}
		}
	}

	// Unicast
	auto it = SwitchingTable.find(ToMAC);
	if (it != SwitchingTable.end())
	{
		BYTE SendBuffer[4096];
		memcpy(SendBuffer, "ETHN", 4);
		memcpy(SendBuffer + 4, Buffer, len);
		Socket.SendToPeer(it->second.Addr, SendBuffer, len + 4);
		printf("Packet to %02X:%02X:%02X:%02X:%02X:%02X unicasted\n", ToMAC.bytes[0], ToMAC.bytes[1], ToMAC.bytes[2], ToMAC.bytes[3], ToMAC.bytes[4], ToMAC.bytes[5]);
		return;
	}
	// Broadcast
	for (auto &P : PeerList)
	{
		if (P.PreferredAddress)
		{
			BYTE SendBuffer[4096];
			memcpy(SendBuffer, "ETHN", 4);
			memcpy(SendBuffer + 4, Buffer, len);
			Socket.SendToPeer(P.PreferredAddress->Address, SendBuffer, len + 4);
		}
	}
	printf("Packet to %02X:%02X:%02X:%02X:%02X:%02X broadcasted\n", ToMAC.bytes[0], ToMAC.bytes[1], ToMAC.bytes[2], ToMAC.bytes[3], ToMAC.bytes[4], ToMAC.bytes[5]);
}

DWORD WINAPI Client::ThreadProc(LPVOID lpParameter)
{
	Client* Me = (Client*)lpParameter;
	BYTE ReadBuffer[4096];
	OVERLAPPED OL = { 0 };
	while (!ReadFile(Me->Device, ReadBuffer, sizeof(ReadBuffer), NULL, &OL))
	{
		WaitForSingleObject(Me->Device, INFINITE);
		if (OL.InternalHigh)
		{
			if (Me->hWnd)
			{
				SendMessage(Me->hWnd, WM_USER + 1, OL.InternalHigh, (LPARAM)ReadBuffer);
			}
		}
		memset(&OL, 0, sizeof(OL));
	}
	Me->Thread = INVALID_HANDLE_VALUE;
	return 0;
}