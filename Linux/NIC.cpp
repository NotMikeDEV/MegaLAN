#include "Globals.h"

NIC::NIC(const char* InterfaceName, NIC_PACKET_CALLBACK Callback, void* CallbackData)
{
	ReadCallback = Callback;
	ReadCallbackData = CallbackData;
	if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
	{
		perror("Error opening TunTap device.");
		exit(-1);
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TAP;
	if (InterfaceName && *InterfaceName)
		strncpy(ifr.ifr_name, InterfaceName, IFNAMSIZ);
	int err;
	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
	{
		close(fd);
	}
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
	{
		printf("hwaddr error.\n");
		exit(-1);
	}
	memcpy(MAC, ifr.ifr_hwaddr.sa_data, 6);
	strcpy(Name, ifr.ifr_name);
	fcntl(fd, F_SETFL, O_NONBLOCK);
	printf("Created %s MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", ifr.ifr_name,
		MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
}

bool NIC::ReadPacket()
{
	unsigned char Buffer[4096];
	int ret = read(fd, Buffer, sizeof(Buffer));
	if (ret > 0)
	{
		ReadCallback(ReadCallbackData, Buffer, ret);
		return true;
	}
	return false;
}

const unsigned char* NIC::GetMAC()
{
	return MAC;
}
const char* NIC::GetName()
{
	return Name;
}

NIC::~NIC()
{
}
