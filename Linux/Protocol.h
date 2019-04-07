#pragma once
typedef void LISTING_CALLBACK(const void* CallerData, const unsigned char ID[20], std::string Name, std::string Description, bool Password);
typedef void VLAN_SERVER_PACKET_CALLBACK(const void* CallerData, const unsigned char Type[4], const unsigned char* Data, int DataLength);
typedef void VLAN_PEER_PACKET_CALLBACK(const void* CallerData, const sockaddr_in6 Addr, const unsigned char Type[4], const unsigned char* Data, int DataLength);

class Protocol
{
private:
	char AuthStatus;
	uint16_t Port;
	std::string Servers;
	sockaddr_in6 PreferredServer;
	std::vector<struct sockaddr_in6> MyExternalAddresses;
	Crypto* ServerCrypto;
	unsigned char PasswordSHA1[20];
	unsigned char PasswordSHA256[32];
	LISTING_CALLBACK* ListingCallback;
	const void* ListingCallbackData = NULL;
	bool ListingComplete = false;
	int ListingInfoPending = 0;
	VLAN_SERVER_PACKET_CALLBACK* VLANCallback = NULL;
	VLAN_PEER_PACKET_CALLBACK* VLANp2pCallback = NULL;
	const void* VLANCallbackData = NULL;
	std::string VLANPassword;
	Crypto* VLANCrypto = NULL;
	void SendRawToServers(std::string Servers, const unsigned char* Buffer, int Length);
public:
	unsigned char UserID[20];
	unsigned char VLANID[20];
	int Socket = -1;
	unsigned char VLANStatus = 0;
	Protocol(const std::string &Servers, const unsigned char UserID[20], const unsigned char PasswordSHA1[20], const unsigned char PasswordSHA256[32]);
	bool RecvPacket();
	bool Auth();
	void GetListing(const LISTING_CALLBACK Callback, const void* CallerData);
	void JoinVLAN(const std::string &VLANName, const std::string &VLANPassword, const VLAN_SERVER_PACKET_CALLBACK Callback, const VLAN_PEER_PACKET_CALLBACK P2PCallback, const void* CallerData);
	void SendRGST(const unsigned char* MAC);
	void SendP2P(sockaddr_in6 Addr, const char Type[4], const unsigned char* Data, int Length);
	~Protocol();
};

