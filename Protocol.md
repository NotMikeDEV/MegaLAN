# Protocol Specification

## Client / Server communication

Format is [Field Name:Length]

**UserID** is the SHA1 of the username.

**PasswordSHA1** is the SHA1 of the username concatinated with the password

**AuthToken** is the SHA256 of the username concatinated with the password

**Bold parts are encrypted with AuthToken**

Strings are NULL-terminated UTF8. (noted as N bytes)

### Authentication

Client > Server

["AUTH":4][UserID:20][PasswordSHA1:20]

Server > Client (Success)

["AUTH":4]**["OK":2][Client IP:16][Client Port:2]**

Server > Client (Failure)

["AUTH":4]

### VLAN Listing

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

### VLAN Create

Client > Server

["MAKE":4][UserID:20][Name:N]

Server > Client

**["OPEN":4][URL:N]**

### Join VLAN

Client > Server

["JOIN":4][UserID:20][VLAN Name:N]

Server > Client

**["JOIN":4][VLANID:20][PasswordOrNot:1][Name:N]**

or

**["EROR":4][Error Message:N]**

#### Register with VLAN

Sent on intial connect, and periodically to refresh new peers. (Currently at least every 5 minuts)

Client > Server

["RGST":4][UserID:20][VLANID:20]

Server > Client

["RGST":4]**[VLANID:20][IPv4:4][IPv4 Prefix:1][IPv6:16][IPv6 Prefix:1]**

Followed by the server sending Peer Notifications for and to each peer.

#### Peer Notification

IPv4 addresses are sent as "IPv4 mapped addresses", aka IPv6 format.

Server > Client

["VLAN":4]**[VLANID:20][UserID:20][MAC:6][IP:16][Port:2]**

#### Peer Identification

Used to get peer name (used in response to initial Peer Notification).

Client > Server

["PEER":4][UserID:20][Peer UserID:20]

Server > Client

["PEER":4]**[Peer UserID:20][Username:N]**