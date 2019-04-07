#include "Globals.h"

Protocol::Protocol(const std::string &Servers, const unsigned char UserID[20], const unsigned char PasswordSHA1[20], const unsigned char PasswordSHA256[32])
{
	ServerCrypto = new Crypto(PasswordSHA256);

	this->Servers = Servers;
	memcpy(this->UserID, UserID, 20);
	memcpy(this->PasswordSHA1, PasswordSHA1, 20);
	memcpy(this->PasswordSHA256, PasswordSHA256, 32);

	struct sockaddr_in6 server_addr = { 0 };
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(0);

	Socket = socket(AF_INET6, SOCK_DGRAM, 0);
	int val = 0;
	setsockopt(Socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&val, sizeof(val));
	if (bind(Socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Error opening UDP Port");
		exit(-1);
	}
	socklen_t addrlen = sizeof(server_addr);
	getsockname(Socket, (struct sockaddr *)&server_addr, &addrlen);
	this->Port = htons(server_addr.sin6_port);
	struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
}

bool Protocol::Auth()
{
	printf("Authenticating UserID ");
	for (unsigned int x = 0; x < 20; x++)
	{
		printf("%02X", UserID[x]);
	}
	printf("\n");
	unsigned char Buffer[44];
	memcpy(Buffer, "AUTH", 4);
	memcpy(Buffer+4, UserID, 20);
	memcpy(Buffer+24, PasswordSHA1, 20);
	SendRawToServers(Servers, (unsigned char*)Buffer, sizeof(Buffer));
	AuthStatus = 0;
	while (RecvPacket() && AuthStatus == 0)
	{
		printf("Oops. Got something else.\n");
	}
	return AuthStatus == 1;
}

void Protocol::GetListing(const LISTING_CALLBACK Callback, const void* CallerData)
{
	ListingCallback = Callback;
	ListingCallbackData = CallerData;
	char Buffer[28];
	memcpy(Buffer, "LIST", 4);
	memcpy(Buffer + 4, UserID, 20);
	memset(Buffer + 24, 0, 4);
	sendto(Socket, Buffer, sizeof(Buffer), 0, (sockaddr*)&PreferredServer, sizeof(PreferredServer));
	while (RecvPacket() && (!ListingComplete || ListingInfoPending > 0));
}

void Protocol::JoinVLAN(const std::string &VLANName, const std::string &VLANPassword, VLAN_SERVER_PACKET_CALLBACK Callback, const VLAN_PEER_PACKET_CALLBACK P2PCallback, const void* CallerData)
{
	this->VLANPassword = VLANPassword;
	VLANCallback = Callback;
	VLANp2pCallback = P2PCallback;
	VLANCallbackData = CallerData;
	char Buffer[1024];
	memcpy(Buffer, "JOIN", 4);
	memcpy(Buffer + 4, UserID, 20);
	strcpy(Buffer + 24, VLANName.c_str());
	sendto(Socket, Buffer, VLANName.size() + 24, 0, (sockaddr*)&PreferredServer, sizeof(PreferredServer));
	while (RecvPacket() && VLANStatus == 0);
}

void Protocol::SendRGST(const unsigned char* MAC)
{
	unsigned char Buffer[4096];
	memcpy(Buffer, "RGST", 4);
	memcpy(Buffer + 4, UserID, 20);
	memcpy(Buffer + 24, VLANID, 20);
	unsigned char Request[4096];
	memcpy(Request, "LETMEIN", 7);
	memcpy(Request + 7, MAC, 6);
	Request[13] = MyExternalAddresses.size();
	for (int x=0; x < MyExternalAddresses.size(); x++)
	{
		memcpy(Request + 14 + (x * 18), &MyExternalAddresses[x].sin6_addr, 16);
		memcpy(Request + 14 + (x * 18) + 16, &MyExternalAddresses[x].sin6_port, 2);
	}
	int len = VLANCrypto->Encrypt(Request, 14 + (MyExternalAddresses.size()*18), Buffer + 44);
	sendto(Socket, Buffer, 44 + len, 0, (sockaddr*)&PreferredServer, sizeof(PreferredServer));
}

bool Protocol::RecvPacket()
{
	unsigned char Buffer[4096];
	memset(Buffer, 0, sizeof(Buffer));
	sockaddr_in6 from;
	socklen_t fromlen = sizeof(from);
	int ret = recvfrom(Socket, Buffer, sizeof(Buffer), 0, (sockaddr*)&from, &fromlen);
	if (ret)
	{
		char IP[128];
		inet_ntop(AF_INET6, &from.sin6_addr, IP, sizeof(IP));
		if (memcmp(Buffer, "AUTH", 4) == 0)
		{
			if (AuthStatus == 0)
			{
				AuthStatus = 2;
				memcpy(&PreferredServer, &from, sizeof(PreferredServer));
			}
			if (ret >= 24)
			{
				unsigned char* DecryptedBuffer = (unsigned char*)malloc(ret);
				int len = ServerCrypto->Decrypt(Buffer + 4, ret - 4, DecryptedBuffer);
				if (len >= 20)
				{
					if (memcmp(DecryptedBuffer, "OK", 2) == 0)
					{
						AuthStatus = 1;
						sockaddr_in6 Addr = { 0 };
						Addr.sin6_family = AF_INET6;
						memcpy(&Addr.sin6_addr, DecryptedBuffer + 2, 16);
						memcpy(&Addr.sin6_port, DecryptedBuffer + 18, 2);
						bool GotIP = false;
						for (auto A : MyExternalAddresses)
						{
							if (memcmp(&A, &Addr, sizeof(Addr)) == 0)
								GotIP = true;
						}
						if (!GotIP)
						{
							MyExternalAddresses.push_back(Addr);
							char IP[128];
							inet_ntop(AF_INET6, &Addr.sin6_addr, IP, sizeof(IP));
							printf("External IP: %s\n", IP);
						}
					}
				}
				free(DecryptedBuffer);
			}
		}
		else if ((memcmp(Buffer, "RGST", 4) == 0 || memcmp(Buffer, "VLAN", 4) == 0))
		{
			unsigned char* DecryptedBuffer = (unsigned char*)malloc(ret);
			if (ret > 4)
			{
				int len = ServerCrypto->Decrypt(Buffer + 4, ret - 4, DecryptedBuffer);
				if (len > 2)
					VLANCallback(VLANCallbackData, Buffer, DecryptedBuffer + 20, len - 20);
			}
			free(DecryptedBuffer);
		}
		else if (memcmp(&from, &PreferredServer, sizeof(from)) == 0)
		{
			unsigned char* DecryptedBuffer = (unsigned char*)malloc(ret);
			int len = ServerCrypto->Decrypt(Buffer, ret, DecryptedBuffer);
			if (len > 4 && memcmp(DecryptedBuffer, "LIST", 4) == 0)
			{
				unsigned char Count = DecryptedBuffer[4];
				if (Count)
				{
					for (int x = 0; x < Count; x++)
					{
						unsigned char* VLANID = DecryptedBuffer + 5 + (x * 20);
						memcpy(Buffer, "INFO", 4);
						memcpy(Buffer + 4, UserID, 20);
						memcpy(Buffer + 24, VLANID, 20);
						sendto(Socket, Buffer, 44, 0, (sockaddr*)&PreferredServer, sizeof(PreferredServer));
						ListingInfoPending++;
					}
					memcpy(Buffer, "LIST", 4);
					memcpy(Buffer + 4, UserID, 20);
					memcpy(Buffer + 24, DecryptedBuffer + 5 + (Count * 20), 4);
					sendto(Socket, Buffer, 28, 0, (sockaddr*)&PreferredServer, sizeof(PreferredServer));
				}
				else
				{
					ListingComplete = true;
				}
			}
			else if (len > 24 && ListingCallback && memcmp(DecryptedBuffer, "INFO", 4) == 0)
			{
				char* Name = (char*)DecryptedBuffer + 24;
				char* Description = (char*)DecryptedBuffer + 25 + strlen(Name);
				unsigned char Password = DecryptedBuffer[26 + strlen(Name) + strlen(Description)];
				ListingCallback(ListingCallbackData, DecryptedBuffer + 4, Name, Description, Password == 0);
				ListingInfoPending--;
			}
			else if (len > 25 && VLANCallback && memcmp(DecryptedBuffer, "EROR", 4) == 0)
			{
				unsigned char* Error = &DecryptedBuffer[4];
				printf("%s\n", Error);
				exit(42);
			}
			else if (len > 25 && VLANCallback && memcmp(DecryptedBuffer, "JOIN", 4) == 0)
			{
				memcpy(VLANID, DecryptedBuffer + 4, 20);
				unsigned char Password = DecryptedBuffer[24];
				if (Password && VLANPassword.length())
				{
					unsigned char VLANKey[32];
					SHA256((unsigned char*)VLANPassword.c_str(), VLANPassword.length(), VLANKey);
					VLANCrypto = new Crypto(VLANKey);
				}
				else if (Password)
				{
					printf("VLAN %s requires a password.\n", DecryptedBuffer + 25);
					exit(2);
				}
				else
				{
					unsigned char VLANKey[32];
					memset(VLANKey, 0, sizeof(VLANKey));
					VLANCrypto = new Crypto(VLANKey);
				}
				printf("Joining VLAN %s\n", DecryptedBuffer + 25);
				VLANStatus = 1;
			}
			free(DecryptedBuffer);
		}
		else if (ret >= 24 && VLANCrypto)
		{
			unsigned char* DecryptedData = (unsigned char*)malloc(ret);
			int len = VLANCrypto->Decrypt(Buffer, ret, DecryptedData);
			if (len > 24)
			{
				VLANp2pCallback(VLANCallbackData, from, DecryptedData, DecryptedData + 4, len - 4);
			}
			if (len == 0)
			{
				printf("P2P Failed to decrypt.\n");
			}
			free(DecryptedData);
		}
	}
	return (ret > 0);
}

void Protocol::SendP2P(sockaddr_in6 Addr, const char Type[4], const unsigned char* Data, int Length)
{
	unsigned char Buffer[4096];
	memcpy(Buffer, Type, 4);
	if (Data && Length)
		memcpy(Buffer + 4, Data, Length);
	unsigned char* Encrypted = (unsigned char*)malloc(Length + 4 + 16);
	int Len = VLANCrypto->Encrypt(Buffer, Length + 4, Encrypted);
	sendto(Socket, Encrypted, Len, 0, (sockaddr*)&Addr, sizeof(Addr));
	free(Encrypted);
}

void Protocol::SendRawToServers(std::string Servers, const unsigned char* Buffer, int Length)
{
	std::string Port = "55555";
	if (Servers.find(":", 0) != std::string::npos)
	{
		Port = Servers.substr(Servers.find(":", 0) + 1);
		Servers = Servers.substr(0, Servers.find(":", 0));
	}

	addrinfo* pResult6;
	addrinfo* pResult4;
	addrinfo* CurrentResult;
	addrinfo hints = { 0 };
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = AF_INET6;
	getaddrinfo(Servers.c_str(), Port.c_str(), &hints, &pResult6);
	hints.ai_family = AF_INET;
	getaddrinfo(Servers.c_str(), Port.c_str(), &hints, &pResult4);

	CurrentResult = pResult6;
	while (CurrentResult != NULL)
	{
		sendto(Socket, (char*)Buffer, Length, 0, CurrentResult->ai_addr, CurrentResult->ai_addrlen);
		CurrentResult = CurrentResult->ai_next;
	}
	CurrentResult = pResult4;
	while (CurrentResult != NULL)
	{
		sendto(Socket, (char*)Buffer, Length, 0, CurrentResult->ai_addr, CurrentResult->ai_addrlen);
		CurrentResult = CurrentResult->ai_next;
	}
	if (pResult6)
		freeaddrinfo(pResult6);
	if (pResult4)
		freeaddrinfo(pResult4);
	if (!pResult6 && !pResult4)
	{
		perror("Unable to find server.");
		exit(-1);
	}
}

Protocol::~Protocol()
{
	delete ServerCrypto;
}
