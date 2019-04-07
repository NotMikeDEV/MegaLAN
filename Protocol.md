All communication is over UDP.

Servers listen on port 55555, clients use dynamic ports.

# Client / Server communication

Format is [Field Name:Length]

**UserID** is the SHA1 of the username.

**PasswordSHA1** is the SHA1 of the username concatinated with the password

**AuthToken** is the SHA256 of the username concatinated with the password

**Bold parts are encrypted with AuthToken**

Strings are NULL-terminated UTF8. (noted as N bytes)

## Authentication

Client > Server

["AUTH":4][UserID:20][PasswordSHA1:20]

Server > Client (Success)

["AUTH":4]**["OK":2][Client IP:16][Client Port:2]**

Server > Client (Failure)

["AUTH":4]

## Create Account

Client > Server

["URLA":4]["REGISTER":8]

Server > Client

["URLA:4"][URL:N]

Client should then open the received URL.

## Forgot Password

Client > Server

["URLA":4]["FORGOTPW":8]

Server > Client

["URLA:4"][URL:N]

Client should then open the received URL.

## VLAN Listing

Initial request [Mark] is 0x00000000, client will repeat requests with the Mark from the last response until a [Count] of 0 is received.

Client > Server

["LIST":4][UserID:20][Mark:4]

Server > Client

**["LIST":4][Count:1]{[VLANID:20]:N}[Mark:4]**

For each VLANID received in a LIST, the following is sent:

Client > Server

["INFO":4][UserID:20][VLANID:20]

Server > Client

**["INFO":4][VLANID:20][Name:N][Description:N][PasswordProtected:1]**

## Join VLAN

Client > Server

["JOIN":4][UserID:20][VLAN Name:N]

Server > Client

**["JOIN":4][VLANID:20][PasswordOrNot:1][Name:N]**

or

**["EROR":4][Error Message:N]**

### Register with VLAN

Sent on intial connect, and periodically to refresh new peers. (Currently at least every 5 minuts)

Client > Server (Part of RGST request is encrypted with VLAN Key (SHA256 of password))

["RGST":4][UserID:20][VLANID:20]***["LETMEIN":7][MAC:6][IP Count:1]{[IP:16][Port:2]}:IP Count***

Server > Client

["RGST":4]**[VLANID:20][IPv4:4][IPv4 Prefix:1][IPv6:16][IPv6 Prefix:1]**

or

**["EROR":4][Error Message:N]**

Followed by the server sending Peer Notifications for and to each peer.

### Peer Notification

IPv4 addresses are sent as "IPv4 mapped addresses", aka IPv6 format.

Server > Client

["VLAN":4]**[VLANID:20][UserID:20][MAC:6][IP:16][Port:2]**

### Peer Identification

Used to get peer name (used in response to initial Peer Notification).

Client > Server

["PEER":4][UserID:20][Peer UserID:20]

Server > Client

["PEER":4]**[Peer UserID:20][Username:N]**

# VLAN p2p Communication

All p2p communication is encrypted with the sha256 of the password. If no password is specified then a NULL key is used. (0000...0000)

Initial communication involves exchanging a list of other known peers to suppliment the list received from the server.

## LAN Discovery

LAN Discovery requires the client to listen on port 55555 in addition to its "working port".
Broadcast packets are sent to port 55555 on the subnet broadcast address of all connected interfaces.
LAN Discovery is only used for IPv4.

LAN Discovery Request (broadcast):

["LAND":4][VLANID:20][UserID:20][MAC:6]

LAN Discovery Reply:

["LANR":4][UserID:20][MAC:6][Peer Count:2]{[PeerUserID:20][PeerMAC:6][PeerIP:16][PeerPort:2]}:PeerCount

## Connection

Initial connection request:

["INIT":4][VLANID:20][UserID:20][MAC:6]

INIT reply:

["PONG":4][UserID:20][MAC:6][Peer Count:2]{[PeerUserID:20][PeerMAC:6][PeerIP:16][PeerPort:2]}:PeerCount

Note: Receiving node should add the source IP and port to its list of prospective peers, and send an INIT packet back.

## KeepAlive/HeartBeat

["PING":4][VLANID:20][UserID:20][MAC:6]

PING Reply:

["PONG":4][UserID:20][MAC:6]

## Ethernet Packet

["ETHN":4][EthernetFrame:N]