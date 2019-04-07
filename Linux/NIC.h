#pragma once
typedef void NIC_PACKET_CALLBACK(const void* CallerData, const unsigned char* Data, int DataLength);

class NIC
{
	struct ifreq ifr;
	NIC_PACKET_CALLBACK* ReadCallback = NULL;
	void* ReadCallbackData = NULL;
	char Name[64];
	unsigned char MAC[6];
public:
	int fd;
	NIC(const char* InterfaceName, NIC_PACKET_CALLBACK Callback, void* CallbackData);
	const unsigned char* GetMAC();
	const char* GetName();
	bool ReadPacket();
	~NIC();
};

