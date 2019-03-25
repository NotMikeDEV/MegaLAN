#pragma once
struct NIC {
	std::wstring ID;
	std::wstring Name;
};

class NICSelector
{
private:
	HINSTANCE hInst;
	std::vector<struct NIC> NICList;
	bool Startup;
public:
	struct NIC SelectedNIC;
	NICSelector();
	~NICSelector();
	struct NIC Run();
	static INT_PTR CALLBACK SelectNICWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};
