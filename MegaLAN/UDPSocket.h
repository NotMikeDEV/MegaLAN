#pragma once
struct InboundUDP {
	struct sockaddr_in6 addr;
	int len;
	BYTE* buffer;
};
class UDPSocket
{
private:
	WORD Port;
	SOCKET Socket;
	SOCKET LANSocket;
	HWND p2pWindow = NULL;
	BYTE AuthKey[32];
	BYTE p2pKey[32];
	struct sockaddr_in6 Server = { 0 };
public:
	BYTE AuthID[20];
	void Start();
	void SocketEventMessage();
	void LANSocketEventMessage();
	void SetServer(struct sockaddr_in6* Address);
	void SetAuthID(BYTE* Key);
	void SetAuthKey(BYTE* Key);
	void SendToServer(char Type[4], BYTE* Payload, int PayloadLength);
	void Setp2pKey(BYTE* Key);
	void SendToPeer(struct sockaddr_in6 &Addr, BYTE* Payload, int PayloadLength);
	void SendRawToServers(std::wstring Server, BYTE* Buffer, int Length);
	void SendLogin(std::wstring Username, std::wstring Password, std::wstring Server);
	~UDPSocket();
};

