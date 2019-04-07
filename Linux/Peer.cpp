#include "Globals.h"

Peer::Peer(const unsigned char ID[20], const unsigned char MAC[6])
{
	memcpy(UserID, ID, 20);
	memcpy(this->MAC, MAC, 6);
}

void Peer::RegisterAddress(const struct sockaddr_in6 &Address)
{
	for (auto A : Addresses)
	{
		if (memcmp(&A.Addr.sin6_addr, &Address.sin6_addr, sizeof(Address.sin6_addr)) == 0 && A.Addr.sin6_port == Address.sin6_port)
			return;
	}
	struct PeerAddress A = { 0 };
	memcpy(&A.Addr, &Address, sizeof(Address));
	A.Expire = time(NULL) + 60;
	Addresses.push_back(A);
}

Peer::~Peer()
{
}
