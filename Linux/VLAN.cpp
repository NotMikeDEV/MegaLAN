#include "Globals.h"

VLAN::VLAN(const std::string &Servers, const unsigned char UserID[20], const unsigned char PasswordSHA1[20], const unsigned char PasswordSHA256[32], const std::string &VLANName, const std::string &VLANPassword, const char* InterfaceName)
{
	printf("Connecting to %s\n", Servers.c_str());
	Link = new Protocol(Servers, UserID, PasswordSHA1, PasswordSHA256);
	if (!Link->Auth())
	{
		printf("Server rejected username/password.\n");
		exit(1);
	}
	Link->JoinVLAN(VLANName, VLANPassword, ServerData, PeerData, this);
	MyNIC = new NIC(InterfaceName, NICData, this);
	Link->SendRGST(MyNIC->GetMAC());
	while (Link->VLANStatus < 2 && Link->RecvPacket());
	if (Link->VLANStatus != 2)
	{
		printf("No response from server. Retrying.\n");
		Link->SendRGST(MyNIC->GetMAC());
		while (Link->VLANStatus < 2 && Link->RecvPacket());
		if (Link->VLANStatus != 2)
		{
			printf("No response from server.\n");
			exit(10);
		}
	}
	printf("Joined VLAN.\n");
	time(&LastRGST);
	fcntl(Link->Socket, F_SETFL, O_NONBLOCK);
}

void VLAN::DoLoop()
{
	if (time(NULL) - LastRGST > 60)
	{
		Link->SendRGST(MyNIC->GetMAC());
		time(&LastRGST);
	}
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(Link->Socket, &read_fds);
	FD_SET(MyNIC->fd, &read_fds);
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	select(MAX(Link->Socket, MyNIC->fd) + 1, &read_fds, NULL, NULL, &timeout);
	if (FD_ISSET(Link->Socket, &read_fds))
		while (Link->RecvPacket());
	if (FD_ISSET(MyNIC->fd, &read_fds))
		while (MyNIC->ReadPacket());
	if (LastPoll != time(NULL))
	{
		PollPeers();
		LastPoll = time(NULL);
	}
}

void VLAN::NICData(const void* CallerData, const unsigned char* Data, int DataLength)
{
	VLAN* Me = (VLAN*)CallerData;
	struct MAC ToMAC;
	memcpy(ToMAC.bytes, Data+4, 6);
	auto it = Me->SwitchingTable.find(ToMAC);
	if (it != Me->SwitchingTable.end())
	{
		Me->Link->SendP2P(it->second.Addr, "ETHN", Data+4, DataLength-4);
//		printf("Packet to %02X:%02X:%02X:%02X:%02X:%02X unicasted\n", ToMAC.bytes[0], ToMAC.bytes[1], ToMAC.bytes[2], ToMAC.bytes[3], ToMAC.bytes[4], ToMAC.bytes[5]);
		return;
	}
	// Broadcast
	for (auto &P : Me->Peers)
	{
		if (P->PreferredAddr)
		{
			Me->Link->SendP2P(P->PreferredAddr->Addr, "ETHN", Data+4, DataLength-4);
		}
	}
//	printf("Packet to %02X:%02X:%02X:%02X:%02X:%02X broadcasted\n", ToMAC.bytes[0], ToMAC.bytes[1], ToMAC.bytes[2], ToMAC.bytes[3], ToMAC.bytes[4], ToMAC.bytes[5]);
}

void VLAN::ServerData(const void* CallerData, const unsigned char Type[4], const unsigned char* Data, int DataLength)
{
	VLAN* Me = (VLAN*)CallerData;
	if (memcmp(Type, "RGST", 4) == 0)
	{
		struct in_addr IPv4;
		memcpy(&IPv4.s_addr, Data, 4);
		unsigned char IPv4_PL = Data[4];
		char IP4[128];
		inet_ntop(AF_INET, &IPv4.s_addr, IP4, sizeof(IP4));
		sprintf(IP4 + strlen(IP4), "/%u", IPv4_PL);
		printf("IPv4 Address: %s\n", IP4);
		int status;
		if (fork() == 0)execlp("ip", "ip", "link", "set", Me->MyNIC->GetName(), "up", NULL); else wait(&status);
		if (fork() == 0)execlp("ip", "ip", "addr", "add", IP4, "dev", Me->MyNIC->GetName(), NULL); else wait(&status);
		Me->Link->VLANStatus = 2;
	}
	if (memcmp(Type, "VLAN", 4) == 0)
	{
		const unsigned char* UserID = Data;
		const unsigned char* MAC = Data + 20;
		sockaddr_in6 Address;
		Address.sin6_family = AF_INET6;
		memcpy(&Address.sin6_addr, Data + 26, 16);
		memcpy(&Address.sin6_port, Data + 42, 2);
		Peer* P = Me->GetPeer(UserID, MAC);
		P->RegisterAddress(Address);
	}
}

void VLAN::PeerData(const void* CallerData, const sockaddr_in6 Addr, const unsigned char Type[4], const unsigned char* Data, int DataLength)
{
	VLAN* Me = (VLAN*)CallerData;
	char T[5] = { 0 };
	memcpy(T, Type, 4);
	char IP[128];
	inet_ntop(AF_INET6, &Addr.sin6_addr, IP, sizeof(IP));
	if (memcmp(Type, "ETHN", 4) == 0)
	{
		char Buffer[4096];
		Buffer[0] = 0;
		Buffer[1] = 0;
		Buffer[2] = 8;
		Buffer[3] = 6;
		memcpy(Buffer+4, Data, DataLength);
		write(Me->MyNIC->fd, Buffer, DataLength + 4);

		struct MAC FromMAC;
		memcpy(FromMAC.bytes, Data + 6, 6);
		struct SwitchingTableEntry Entry = { Addr, time(NULL) + 10};
		Me->SwitchingTable[FromMAC] = Entry;
	}
	else if (memcmp(Type, "PONG", 4) == 0)
	{
		Peer* P = Me->GetPeer(Data, Data + 20);
		P->RegisterAddress(Addr);
		for (auto &A : P->Addresses)
		{
			if (memcmp(&A.Addr.sin6_addr, &Addr.sin6_addr, 16) == 0 && A.Addr.sin6_port == Addr.sin6_port)
			{
				A.LastRecv = time(NULL);
				A.Expire = time(NULL) + 120;
			}
		}
		if (DataLength > 28)
		{
			uint16_t Count = htons(*(uint16_t*)(Data + 26));
			for (int x = 0; x < Count; x++)
			{
				const unsigned char* PeerID = Data + 28 + (x * 44);
				const unsigned char* PeerMAC = Data + 48 + (x * 44);
				sockaddr_in6 PeerAddr;
				PeerAddr.sin6_family = AF_INET6;
				memcpy(&PeerAddr.sin6_addr, Data + 54 + (x * 44), 16);
				memcpy(&PeerAddr.sin6_port, Data + 70 + (x * 44), 2);
				Peer* P = Me->GetPeer(PeerID, PeerMAC);
				P->RegisterAddress(PeerAddr);
			}
		}
	}
	else if (memcmp(Type, "INIT", 4) == 0 || memcmp(Type, "PING", 4) == 0)
	{
		if (memcmp(Data, Me->Link->VLANID, 20) != 0)
			return;
		if (memcmp(Data + 20, Me->Link->UserID, 20) == 0 && memcmp(Data + 40, Me->MyNIC->GetMAC(), 6) == 0)
			return;
		
		Peer* P = Me->GetPeer(Data+20, Data + 40);
		P->RegisterAddress(Addr);
		unsigned char Buffer[4096];
		memcpy(Buffer, Me->Link->UserID, 20);
		memcpy(Buffer + 20, Me->MyNIC->GetMAC(), 6);
		*(uint16_t*)(Buffer + 26) = 0;
		if (memcmp(Type, "PING", 4) == 0)
		{
			Me->Link->SendP2P(Addr, "PONG", Buffer, 28);
		}
		else
		{
			uint16_t Count = 0;
			for (auto P : Me->Peers)
			{
				if (P->PreferredAddr && P->PreferredAddr->LastRecv && Count < 50)
				{
					memcpy(Buffer + 28 + (Count * 44), P->UserID, 20);
					memcpy(Buffer + 28 + (Count * 44) + 20, P->MAC, 6);
					memcpy(Buffer + 28 + (Count * 44) + 26, &P->PreferredAddr->Addr.sin6_addr, 16);
					memcpy(Buffer + 28 + (Count * 44) + 42, &P->PreferredAddr->Addr.sin6_port, 2);
					Count++;
				}
			}
			*(uint16_t*)(Buffer + 26) = htons(Count);
			Me->Link->SendP2P(Addr, "PONG", Buffer, 48 + (Count * 44));
		}
	}
	else
	{
		printf("%s from %s %u\n", T, IP, htons(Addr.sin6_port));
	}
}

Peer* VLAN::GetPeer(const unsigned char UserID[20], const unsigned char MAC[6])
{
	for (auto P : Peers)
	{
		if (memcmp(P->UserID, UserID, 20) == 0 && memcmp(P->MAC, MAC, 6) == 0)
			return P;
	}
	Peer* P = new Peer(UserID, MAC);
	Peers.push_back(P);
	return P;
}

Peer* VLAN::GetPeer(const struct sockaddr_in6 &Addr)
{
	for (auto P : Peers)
	{
		for (auto &A : P->Addresses)
		{
			if (memcmp(&A.Addr.sin6_addr, &Addr.sin6_addr, 16) == 0 && A.Addr.sin6_port == Addr.sin6_port)
				return P;
		}
	}
	return NULL;
}

void VLAN::PollPeers()
{
	time_t Time = time(NULL);
	for (int x=0; x < Peers.size(); x++)
	{
		Peers[x]->PreferredAddr = NULL;
		for (int y=0; y < Peers[x]->Addresses.size(); y++)
		{
			if (!Peers[x]->PreferredAddr && Peers[x]->Addresses[y].LastRecv)
				Peers[x]->PreferredAddr = &Peers[x]->Addresses[y];

			if (Peers[x]->Addresses[y].Expire < Time)
			{
				Peers[x]->Addresses.erase(Peers[x]->Addresses.begin() + y);
				y--;
			}
			else if (Peers[x]->Addresses[y].LastPing < Time - 30 && Peers[x]->Addresses[y].LastRecv == 0) // send INIT
			{
				unsigned char Buffer[46];
				memcpy(Buffer, Link->VLANID, 20);
				memcpy(Buffer + 20, Link->UserID, 20);
				memcpy(Buffer + 40, MyNIC->GetMAC(), 6);
				Link->SendP2P(Peers[x]->Addresses[y].Addr, "INIT", Buffer, 46);
				Peers[x]->Addresses[y].LastPing = Time;
			}
			else if (Peers[x]->Addresses[y].LastPing < Time - 30) // send PING
			{
				unsigned char Buffer[46];
				memcpy(Buffer, Link->VLANID, 20);
				memcpy(Buffer + 20, Link->UserID, 20);
				memcpy(Buffer + 40, MyNIC->GetMAC(), 6);
				Link->SendP2P(Peers[x]->Addresses[y].Addr, "PING", Buffer, 46);
				Peers[x]->Addresses[y].LastPing = Time;
			}
		}
		if (Peers[x]->Addresses.size() == 0)
		{
			Peers.erase(Peers.begin() + x);
			x--;
		}
	}
	for (auto it = SwitchingTable.begin(); it != SwitchingTable.end(); it++)
	{
		if (it->second.Timeout < time(NULL))
		{
			SwitchingTable.erase(it);
			return;
		}
	}
	return;
	printf("\e[1;1H\e[2J");
	for (int x = 0; x < Peers.size(); x++)
	{
		printf("Peer ");
		for (int z = 0; z < 20; z++)
			printf("%02X", Peers[x]->UserID[z]);
		printf(" MAC ");
		for (int z = 0; z < 6; z++)
			printf("%02X", Peers[x]->MAC[z]);
		printf("\n");
		for (int y = 0; y < Peers[x]->Addresses.size(); y++)
		{
			char IP[128];
			inet_ntop(AF_INET6, &Peers[x]->Addresses[y].Addr.sin6_addr, IP, sizeof(IP));
			printf("%s %u  expire %d lastrecv %d\n", IP, htons(Peers[x]->Addresses[y].Addr.sin6_port), Peers[x]->Addresses[y].Expire, Peers[x]->Addresses[y].LastRecv);
		}
	}
}

VLAN::~VLAN()
{
}
