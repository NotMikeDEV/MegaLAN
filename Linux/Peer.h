#pragma once
struct PeerAddress {
	sockaddr_in6 Addr;
	time_t LastPing;
	time_t LastRecv;
	time_t Expire;
};
class Peer
{
public:
	PeerAddress* PreferredAddr;
	std::vector<struct PeerAddress> Addresses;
	unsigned char UserID[20];
	unsigned char MAC[6];
	Peer(const unsigned char ID[20], const unsigned char MAC[6]);
	void RegisterAddress(const struct sockaddr_in6 &Address);
	~Peer();
};

