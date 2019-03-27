struct PeerAddrInfo {
	struct sockaddr_in6 Address;
	BYTE State;
	UINT16 Latency;
	UINT64 LastCheckTime;
	UINT64 LastRecvTime;
	WCHAR IPString[192];
	struct {
		bool FromServer;
		bool FromPeer;
		bool FromLAN;
		bool FromINIT;
	} DiscoveryFlags;
};
class Peer
{
public:
	std::vector<struct PeerAddrInfo> Addresses;
	UINT64 Timeout;
	BYTE UserID[20];
	struct PeerAddrInfo* PreferredAddress;
	BYTE MAC[6];
	std::wstring Name;
	Peer(BYTE* UserID, BYTE* MAC);
	bool operator==(Peer &Other);
	void RegisterAddress(const struct in_addr6 &Address, UINT16 Port, UINT DiscoverySource);
	void Poll();
	void Pong(const struct sockaddr_in6 &Addr);
	~Peer();
};
