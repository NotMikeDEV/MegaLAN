#include "NICSelector.h"
#include "UDPSocket.h"
#include "Login.h"
#include "Peer.h"
#include "Client.h"

extern HINSTANCE hInst;
extern NOTIFYICONDATA trayIcon;
extern HWND hWnd;
extern NICSelector NICs;
extern UDPSocket Socket;
extern Login Session;
extern Client* VPNClient;

extern std::vector<struct sockaddr_in6> MyExternalAddresses;
void RecvPacket(struct InboundUDP &Packet);
#define LAN_DISCOVERY_PORT 55555