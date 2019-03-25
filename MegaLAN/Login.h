#pragma once
class Login
{
private:
	HWND hDlg;
	std::wstring Username;
	std::wstring Password;
	bool IsLoggingIn;
	bool IsAuthenticated;
public:
	Login();
	~Login();
	void Show();
	void RecvPacket(struct InboundUDP &Packet);
	static INT_PTR CALLBACK LoginWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};
