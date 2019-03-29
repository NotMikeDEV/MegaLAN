#include "stdafx.h"
#include <string>
#include <codecvt>
#include <locale>
#include "MegaLAN.h"
#include "Crypto.h"

void UDPSocket::Start()
{
	// Bind to dynamic port
	WSADATA WSAData;
	memset(&WSAData, 0, sizeof(WSAData));
	WSAData.iMaxSockets = 10000;
	if (WSAStartup(MAKEWORD(1, 1), &WSAData))
	{
		MessageBox(0, L"ERROR: Unable to initialise WinSock", 0, 0);
		return;
	}

	struct sockaddr_in6 server_addr = { 0 };
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(0);

	Socket = socket(AF_INET6, SOCK_DGRAM, 0);
	int val = 0;
	setsockopt(Socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&val, sizeof(val));
	if (bind(Socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		int err = WSAGetLastError();
		WCHAR Error[1024] = { 0 };
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, Error, sizeof(Error)/2, NULL);
		MessageBox(NULL, Error, L"Error opening UDP Port.", 0);
		exit(-1);
	}
	socklen_t addrlen = sizeof(server_addr);
	getsockname(Socket, (struct sockaddr *)&server_addr, &addrlen);
	this->Port = htons(server_addr.sin6_port);
	WSAAsyncSelect(Socket, hWnd, WM_USER + 100, FD_READ);

	// LAN Discovery listener, fixed port.
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(LAN_DISCOVERY_PORT);
	LANSocket = socket(AF_INET6, SOCK_DGRAM, 0);
	val = 0;
	setsockopt(LANSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&val, sizeof(val));
	if (val = bind(LANSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
	{
		WSAAsyncSelect(LANSocket, hWnd, WM_USER + 0xFF, FD_READ);
	}
}

void UDPSocket::LANSocketEventMessage()
{
	struct sockaddr_in6 from = { 0 };
	from.sin6_family = AF_INET6;
	int fromlen = sizeof(from);
	unsigned char Buffer[4096];
	memset(Buffer, 0, sizeof(Buffer));
	int ret = recvfrom(LANSocket, (char*)Buffer, sizeof(Buffer), 0, (sockaddr*)&from, &fromlen);

	if (VPNClient && (ret = Crypt.AES256_Decrypt(Buffer, ret, p2pKey)))
	{
		char IP[128];
		inet_ntop(AF_INET6, &from.sin6_addr, IP, sizeof(IP));
		char Type[5] = { 0 };
		memcpy(Type, Buffer, 4);
		printf("Recv %u byte %s from peer %s %u on LAN discovery port\n", ret, Type, IP, htons(from.sin6_port));
		for (int x = 0; x < ret; x++)
			printf("%02X ", (BYTE)Buffer[x]);
		printf("\n");
		struct InboundUDP PacketInfo;
		PacketInfo.addr = from;
		PacketInfo.len = ret;
		PacketInfo.buffer = Buffer;
		VPNClient->RecvPacket(PacketInfo);
	}
}
void UDPSocket::SocketEventMessage()
{
	struct sockaddr_in6 from = { 0 };
	from.sin6_family = AF_INET6;
	int fromlen = sizeof(from);
	unsigned char Buffer[4096];
	memset(Buffer, 0, sizeof(Buffer));
	int ret = recvfrom(Socket, (char*)Buffer, sizeof(Buffer), 0, (sockaddr*)&from, &fromlen);
	if (ret >= 4 && memcmp(Buffer, "URLA", 4) == 0)
	{
		printf("Recv URLA %s (%u bytes) from server\n", Buffer + 4, ret);
		struct InboundUDP PacketInfo;
		PacketInfo.addr = from;
		PacketInfo.len = ret;
		PacketInfo.buffer = Buffer;
		Session.RecvPacket(PacketInfo);
	}
	if (ret >= 4 && memcmp(Buffer, "AUTH", 4) == 0)
	{
		printf("Recv %c%c%c%c (%u bytes) from server\n", Buffer[0], Buffer[1], Buffer[2], Buffer[3], ret);
		if (ret = Crypt.AES256_Decrypt(Buffer + 4, ret - 4, AuthKey))
		{
			for (int x = 0; x < ret + 4; x++)
				printf("%02X ", (BYTE)Buffer[x]);
			printf("\n");
			struct InboundUDP PacketInfo;
			PacketInfo.addr = from;
			PacketInfo.len = ret - 4;
			PacketInfo.buffer = Buffer + 4;
			Session.RecvPacket(PacketInfo);
			PacketInfo.len = ret + 4;
			PacketInfo.buffer = Buffer;
			RecvPacket(PacketInfo);
		}
		else
		{
			struct InboundUDP PacketInfo;
			Session.RecvPacket(PacketInfo);
		}
	}
	else if (VPNClient && (memcmp(Buffer, "RGST", 4) == 0 || memcmp(Buffer, "VLAN", 4) == 0 || memcmp(Buffer, "PEER", 4) == 0))
	{
		printf("Recv (%u bytes) for VPN Client\n", ret);
		if (ret = Crypt.AES256_Decrypt(Buffer + 4, ret - 4, AuthKey))
		{
			for (int x = 0; x < ret + 4; x++)
				printf("%02X ", (BYTE)Buffer[x]);
			printf("\n");
			struct InboundUDP PacketInfo;
			PacketInfo.addr = from;
			PacketInfo.len = ret + 4;
			PacketInfo.buffer = Buffer;
			VPNClient->RecvPacketFromServer(PacketInfo);
		}
	}
	else if (ret > 0 && memcmp(&from, &Server, sizeof(Server)) == 0)
	{
		if (ret = Crypt.AES256_Decrypt(Buffer, ret, AuthKey))
		{
			char Type[5] = { 0 };
			memcpy(Type, Buffer, 4);
			printf("Recv %u byte %s from server\n", ret, Type);
			for (int x = 0; x < ret; x++)
				printf("%02X ", (BYTE)Buffer[x]);
			printf("\n");
			struct InboundUDP PacketInfo;
			PacketInfo.addr = from;
			PacketInfo.len = ret;
			PacketInfo.buffer = Buffer;
			RecvPacket(PacketInfo);
		}
	}
	else if (ret > 0)
	{
		if (VPNClient && (ret = Crypt.AES256_Decrypt(Buffer, ret, p2pKey)))
		{
			char IP[128];
			inet_ntop(AF_INET6, &from.sin6_addr, IP, sizeof(IP));
			char Type[5] = { 0 };
			memcpy(Type, Buffer, 4);
			printf("Recv %u byte %s from peer %s %u\n", ret, Type, IP, htons(from.sin6_port));
			for (int x = 0; x < ret; x++)
				printf("%02X ", (BYTE)Buffer[x]);
			printf("\n");
			struct InboundUDP PacketInfo;
			PacketInfo.addr = from;
			PacketInfo.len = ret;
			PacketInfo.buffer = Buffer;
			VPNClient->RecvPacket(PacketInfo);
		}
	}
}
void UDPSocket::SetServer(struct sockaddr_in6* Address)
{
	memcpy(&Server, Address, sizeof(Server));
}
void UDPSocket::SetAuthID(BYTE* Key)
{
	memcpy(AuthID, Key, sizeof(AuthID));
}
void UDPSocket::SetAuthKey(BYTE* Key)
{
	memcpy(AuthKey, Key, sizeof(AuthKey));
}
void UDPSocket::SendToServer(char Type[4], BYTE* Payload, int PayloadLength)
{
	BYTE Buffer[4096] = { 0 };
	memcpy(Buffer, Type, 4);
	memcpy(Buffer + 4, AuthID, 20);
	char TypeT[5] = { 0 };
	memcpy(TypeT, Type, 4);
	printf("Sending %s %d (%d) to server from ", Type, PayloadLength, 24 + PayloadLength);
	for (int x = 0; x < sizeof(AuthID); x++)
		printf("%02X ", AuthID[x]);
	printf("\n");
	if (PayloadLength)
	{
		memcpy(Buffer + 24, Payload, PayloadLength);
	}
	sendto(Socket, (char*)Buffer, 24+PayloadLength, 0, (struct sockaddr*)&Server, sizeof(Server));
}

void UDPSocket::Setp2pKey(BYTE* Key)
{
	memcpy(p2pKey, Key, sizeof(p2pKey));
}

void UDPSocket::SendToPeer(struct sockaddr_in6 &Addr, BYTE* Payload, int PayloadLength)
{
	BYTE Buffer[4096] = { 0 };
	char IP[128];
	inet_ntop(AF_INET6, &Addr.sin6_addr, IP, sizeof(IP));
	char TypeT[5] = { 0 };
	memcpy(TypeT, Payload, 4);
	printf("Sending %s %d to peer %s %u with key ", TypeT, PayloadLength, IP, htons(Addr.sin6_port));
	for (int x = 0; x < sizeof(p2pKey); x++)
		printf("%02X ", p2pKey[x]);
	printf("\n");
	if (PayloadLength)
	{
		memcpy(Buffer, Payload, PayloadLength);
	}
	int Len = Crypt.AES256_Encrypt(Buffer, PayloadLength, p2pKey);
	sendto(Socket, (char*)Buffer, Len, 0, (struct sockaddr*)&Addr, sizeof(Addr));
}

void UDPSocket::SendRawToServers(std::wstring Server, BYTE* Buffer, int Length)
{
	PADDRINFOW pResult, CurrentResult;
	ADDRINFOW hints = { 0 };
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET6;
	hints.ai_flags = AI_V4MAPPED | AI_ALL;
	std::wstring Port = L"55555";
	if (Server.find(L":", 0) != std::string::npos)
	{
		Port = Server.substr(Server.find(L":", 0) + 1);
		Server = Server.substr(0, Server.find(L":", 0));
	}
	GetAddrInfo(Server.c_str(), Port.c_str(), &hints, &pResult);
	CurrentResult = pResult;
	while (CurrentResult)
	{
		struct sockaddr_in6 from = { 0 };
		sendto(Socket, (char*)Buffer, Length, 0, CurrentResult->ai_addr, CurrentResult->ai_addrlen);
		CurrentResult = CurrentResult->ai_next;
	}
	if (pResult)
		FreeAddrInfoW(pResult);
	else
		MessageBox(NULL, L"Unable to find server.", Server.c_str(), 0);
}

void UDPSocket::SendLogin(std::wstring Username, std::wstring Password, std::wstring Server)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
	std::string User = utf8_conv.to_bytes(Username);
	std::string Pass = utf8_conv.to_bytes(Password);
	BYTE Buffer[1024] = { 0 };
	std::string Login(User);
	Login.append(Pass);
	memcpy(Buffer, "AUTH", 4);
	BYTE* SHA1 = Crypt.SHA1(User);
	memcpy(Buffer + 4, SHA1, 20);
	SetAuthID(SHA1);
	SHA1 = Crypt.SHA1(Login);
	memcpy(Buffer + 24, SHA1, 20);
	BYTE* SHA256 = Crypt.SHA256(Login);
	SetAuthKey(SHA256);
	SendRawToServers(Server, Buffer, 44);
	MyExternalAddresses.clear();
}

UDPSocket::~UDPSocket()
{
}
