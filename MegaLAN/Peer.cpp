#include "stdafx.h"
#include "MegaLAN.h"

Peer::Peer(BYTE* UserID, BYTE* MAC)
{
	memcpy(this->UserID, UserID, 20);
	memcpy(this->MAC, MAC, 6);
	this->PreferredAddress = NULL;
	this->Timeout = GetTickCount64() + 70000;
	printf("New Peer ");
	for (int x = 0; x < 20; x++)
		printf("%02X", this->UserID[x]);
	for (int x = 0; x < 6; x++)
		printf(":%02X", this->MAC[x]);
	printf("\n");
}

bool Peer::operator==(Peer &Other)
{
	if (memcmp(UserID, Other.UserID, 20) == 0 && memcmp(MAC, Other.MAC, 6) == 0)
		return true;
	return false;
}

void Peer::RegisterAddress(const struct in_addr6 &Address, UINT16 Port)
{
	struct sockaddr_in6 Addr = {0};
	Addr.sin6_family = AF_INET6;
	memcpy(&Addr.sin6_addr, &Address, sizeof(Address));
	Addr.sin6_port = htons(Port);
	for (auto &A : Addresses)
	{
		if (memcmp(&A.Address, &Addr, sizeof(Addr)) == 0)
		{
			if (A.State == 0)
				A.LastCheckTime = 0;
			return;
		}
	}
	struct PeerAddrInfo Info = { 0 };
	memcpy(&Info.Address, &Addr, sizeof(Addr));
	Addresses.push_back(Info);
	char IP[128];
	inet_ntop(AF_INET6, &Info.Address.sin6_addr, IP, sizeof(IP));
	printf("Register IP for ");
	for (int x = 0; x < 20; x++)
		printf("%02X", this->UserID[x]);
	for (int x = 0; x < 20; x++)
		printf(":%02X", this->UserID[x]);
	printf("\n");
}

void Peer::Poll()
{
	UINT64 Time = GetTickCount64();
	UINT64 LowestLatency = -1;
	PreferredAddress = NULL;
	for (int index = 0; index < Addresses.size(); ++index)
	{
		if (Name.length() == 0)
		{
			Socket.SendToServer((char*)"PEER", UserID, 20);
		}
		if (Time - Addresses[index].LastCheckTime > 30000)
		{
			BYTE Buffer[1024];
			memcpy(Buffer, "PING", 4);
			memcpy(Buffer + 4, VPNClient->ID, 20);
			memcpy(Buffer + 24, Socket.AuthID, 20);
			memcpy(Buffer + 44, VPNClient->MyMAC, 6);
			Socket.SendToPeer(Addresses[index].Address, Buffer, 50);
			if (Addresses[index].State == 0)
			{
				memcpy(Buffer, "INIT", 4);
				memcpy(Buffer + 4, VPNClient->ID, 20);
				memcpy(Buffer + 24, Socket.AuthID, 20);
				memcpy(Buffer + 44, VPNClient->MyMAC, 6);
				Socket.SendToPeer(Addresses[index].Address, Buffer, 50);
			}
			Addresses[index].LastCheckTime = Time;
		}
		if (Time - Addresses[index].LastRecvTime > 40000)
			Addresses[index].State = 0;
		if (Addresses[index].State > 0 && Addresses[index].Latency < LowestLatency)
		{
			PreferredAddress = &Addresses[index];
			LowestLatency = Addresses[index].Latency;
		}
		if (Time - Addresses[index].LastRecvTime > 120000 && PreferredAddress)
		{
			Addresses.erase(Addresses.begin() + index);
			index--;
		}
	}
	if (PreferredAddress && Time - PreferredAddress->LastCheckTime > 10000)
	{
		BYTE Buffer[1024];
		memcpy(Buffer, "PING", 4);
		memcpy(Buffer + 4, VPNClient->ID, 20);
		memcpy(Buffer + 24, Socket.AuthID, 20);
		memcpy(Buffer + 44, VPNClient->MyMAC, 6);
		Socket.SendToPeer(PreferredAddress->Address, Buffer, 50);
		PreferredAddress->LastCheckTime = Time;
	}
}
void Peer::Pong(const struct sockaddr_in6 &Addr)
{
	for (auto &A : Addresses)
	{
		if (memcmp(&A.Address.sin6_addr, &Addr.sin6_addr, 16) == 0 && A.Address.sin6_port == Addr.sin6_port)
		{
			A.Latency = GetTickCount64() - A.LastCheckTime;
			A.State = 1;
			A.LastRecvTime = GetTickCount64();
			printf("PONG in %ums\n", A.Latency);
			this->Timeout = GetTickCount64() + 120000;
		}
	}
}

Peer::~Peer()
{
}
