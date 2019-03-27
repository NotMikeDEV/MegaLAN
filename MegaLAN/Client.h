struct MAC {
	BYTE bytes[6];
	bool operator == (const struct MAC &other) const
	{
		return bytes[0] == other.bytes[0] &&
			bytes[1] == other.bytes[1] &&
			bytes[2] == other.bytes[2] &&
			bytes[3] == other.bytes[3] &&
			bytes[4] == other.bytes[4] &&
			bytes[5] == other.bytes[5];
	}
	bool operator<(const struct MAC &o) const
	{
		if (bytes[0] < o.bytes[0])
			return true;
		if (bytes[0] > o.bytes[0])
			return false;
		if (bytes[1] < o.bytes[1])
			return true;
		if (bytes[1] > o.bytes[1])
			return false;
		if (bytes[2] < o.bytes[2])
			return true;
		if (bytes[2] > o.bytes[2])
			return false;
		if (bytes[3] < o.bytes[3])
			return true;
		if (bytes[3] > o.bytes[3])
			return false;
		if (bytes[4] < o.bytes[4])
			return true;
		if (bytes[4] > o.bytes[4])
			return false;
		if (bytes[5] < o.bytes[5])
			return true;
		if (bytes[5] > o.bytes[5])
			return false;
		return false;
	}
};
struct SwitchingTableEntry {
	struct sockaddr_in6 Addr;
	UINT64 Timeout;
};
class Client
{
	std::map<struct MAC, struct SwitchingTableEntry> SwitchingTable;
private:
	HWND hDlg = NULL;
	HWND hWnd = NULL;
	std::wstring Name;
	BYTE Key[32];
	HANDLE Device = INVALID_HANDLE_VALUE;
	HANDLE Thread = INVALID_HANDLE_VALUE;
	struct in_addr IPv4_Address = { 0 };
	BYTE IPv4_PrefixLength = 24;
	struct in_addr6 IPv6_Address = { 0 };
	BYTE IPv6_PrefixLength = 64;
	std::vector<Peer> PeerList;
public:
	BYTE ID[20];
	BYTE MyMAC[6];
	Client(BYTE* ID, std::wstring Name);
	~Client();
	void Login(bool Password);
	void Login(BYTE* Key);
	void Start();
	void SendRegister();
	void OpenDevice();
	void RegisterPeer(BYTE* UserID, BYTE* MAC, struct in6_addr &Address, UINT16 Port);
	void RecvPacketFromServer(struct InboundUDP &Packet);
	void RecvPacket(struct InboundUDP &Packet);
	void ReadPacket(BYTE* Buffer, WORD len);
	static INT_PTR CALLBACK PasswordWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static DWORD WINAPI ThreadProc(LPVOID lpParameter);
};

