#pragma once
struct MAC {
	unsigned char bytes[6];
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
	time_t Timeout;
};
class VLAN
{
	time_t LastPoll;
	std::map<struct MAC, struct SwitchingTableEntry> SwitchingTable;
	Protocol* Link;
	NIC* MyNIC;
	time_t LastRGST;
	std::vector<Peer*> Peers;
	void PollPeers();
public:
	VLAN(const std::string &Servers, const unsigned char UserID[20], const unsigned char PasswordSHA1[20], const unsigned char PasswordSHA256[32], const std::string &VLANName, const std::string &VLANPassword, const char* InterfaceName);
	static void NICData(const void* CallerData, const unsigned char* Data, int DataLength);
	static void ServerData(const void* CallerData, const unsigned char Type[4], const unsigned char* Data, int DataLength);
	static void PeerData(const void* CallerData, const sockaddr_in6 Addr, const unsigned char Type[4], const unsigned char* Data, int DataLength);
	Peer* GetPeer(const unsigned char UserID[20], const unsigned char MAC[6]);
	Peer* GetPeer(const struct sockaddr_in6 &Addr);
	void DoLoop();
	~VLAN();
};

