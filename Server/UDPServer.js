#!/usr/bin/nodejs

const dgram = require('dgram');
const crypto = require('crypto');
const IPAddr = require('ip6addr');
var args = process.argv.slice(2);
var ServerName = args[0];

var mysql = require('mysql');
var database = mysql.createPool({
    connectionLimit: 10,
    host: 'localhost',
    user: 'MegaLAN',
    password: 'MegaLAN',
    database: 'MegaLAN'
});

const server = dgram.createSocket('udp6');
server.on('error', (err) => {
	console.log(`server error:\n${err.stack}`);
	server.close();
});
setInterval(function(){
    database.query("DELETE FROM VLAN_Membership WHERE Time < ?", [Math.floor(new Date() / 1000) - 600]);
    database.query("DELETE FROM IP_Allocations WHERE Time < ?", [Math.floor(new Date() / 1000) - 3600]);
}, 60000);
server.on('message', (msg, rinfo) => {
	var Buff = Buffer.from(msg);
	var Type = Buff.slice(0,4);
	var UserHash = Buff.slice(4,24);
	console.log(`server request: ${Type} from ${UserHash.toString('hex')} ${rinfo.address}:${rinfo.port}`);
	var Start = Buff.slice(24,28);
    database.query("SELECT COUNT(*) as Valid, Username, CryptoHash FROM Accounts WHERE UserHash = ?", [UserHash.toString('hex')], (err, user) => {
		if (err) {
			return console.error(err.message);
		}
		if (Type == "AUTH")
		{
			var Password = Buff.slice(24,44);
            database.query("SELECT COUNT(*) as Valid, Username, CryptoHash FROM Accounts WHERE UserHash = ? AND PasswordHash = ?", [UserHash.toString('hex'), Password.toString('hex')], (err, user) => {
				if (err) {
					return console.error(err.message);
				}
                console.log(`server got login request: ${Type} from ${user[0].Username} (${user[0].Valid}) ${rinfo.address}:${rinfo.port}`);
                if (user[0] && user[0].Valid)
				{
					var ClientIP = IPAddr.parse(rinfo.address);
					var ClientPort = new Buffer(2);
					ClientPort.writeUInt16BE(rinfo.port, 0);
                    var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
					const iv = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
					var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
					var encrypted = cipher.update('OK', 'binary', 'hex');
					encrypted += cipher.update(ClientIP.toBuffer(), 'hex', 'hex');
					encrypted += cipher.update(ClientPort.toString('hex'), 'hex', 'hex');
					encrypted += cipher.final('hex');
					server.send(Buffer.concat([Buffer.from("AUTH"), Buffer.from(encrypted,'hex')]), rinfo.port, rinfo.address);
				}
				else
				{
					server.send("AUTH", rinfo.port, rinfo.address);
				}
			});
		}
        else if (user[0].Valid && Type == "LIST")
		{
            database.query("SELECT ROWID, VLANID FROM VLANs WHERE Public = '1' AND ROWID > ? LIMIT 15;", [Start.readUInt32BE()], function(err, vlans) {
                var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
				const iv = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
				var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
				var encrypted = cipher.update('LIST', 'binary', 'hex');
				encrypted += cipher.update(Buffer([vlans.length]), 'hex', 'hex');
				if (vlans.length)
				{
                    console.log("Sending " + vlans.length + " VLANs (from " + Start.readUInt32BE() + " to " + vlans[vlans.length - 1].ROWID + ")");
					for (var x=0; x<vlans.length; x++)
					{
						encrypted += cipher.update(vlans[x].VLANID, 'hex', 'hex');
					}
					End = new Buffer(4);
					End.writeUInt32BE(vlans[vlans.length-1].ROWID, 0);
					encrypted += cipher.update(End.toString('hex'), 'hex', 'hex');
				}
				encrypted += cipher.final('hex');
				server.send(Buffer.from(encrypted,'hex'), rinfo.port, rinfo.address);
			});
		}
        else if (user[0].Valid && Type == "INFO")
		{
			var VLAN = Buff.slice(24,44);
            database.query("SELECT VLANID, Name, Description, CryptoKey FROM VLAN_Info WHERE VLANID = ?;", [VLAN.toString('hex')], function (err, vlans) {
                if (vlans && vlans.length)
                {
                    var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
                    const iv = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
                    var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
                    var encrypted = cipher.update('INFO', 'binary', 'hex');
                    encrypted += cipher.update(vlans[0].VLANID, 'hex', 'hex');
                    encrypted += cipher.update(vlans[0].Name + "\0", 'utf8', 'hex');
                    encrypted += cipher.update(vlans[0].Description + "\0", 'utf8', 'hex');
                    if (vlans[0].CryptoKey.length)
                        encrypted += cipher.update(Buffer([0]), 'hex', 'hex');
                    else
                        encrypted += cipher.update(Buffer([1]), 'hex', 'hex');
                    encrypted += cipher.final('hex');
                    console.log("Sending info for '" + vlans[0].Name + "' to " + user[0].Username);
                    server.send(Buffer.from(encrypted, 'hex'), rinfo.port, rinfo.address);
                }
			});
		}
        else if (user[0].Valid && Type == "MAKE")
		{
			var Name = Buff.slice(24);
			var Token=crypto.randomBytes(10);
            database.query("UPDATE Accounts SET Token=? WHERE UserHash=?", [Token.toString('hex'), UserHash.toString('hex')], function () {
                var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
                const iv = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
                var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
                var encrypted = cipher.update('OPEN', 'binary', 'hex');
                encrypted += cipher.update('https://' + ServerName + '.megalan.app/create_vlan/?user=' + UserHash.toString('hex') + '&token=' + Token.toString('hex') + '&name=' + Name + "\0", 'utf8', 'hex');
                encrypted += cipher.final('hex');
                server.send(Buffer.from(encrypted, 'hex'), rinfo.port, rinfo.address);
                console.log(`server got create request from ${user[0].Username} for ${Name}`);
            });
		}
		else if (user[0].Valid && Type == "JOIN")
		{
			var Name = Buff.slice(24);
            database.query("SELECT VLANID, CryptoKey FROM VLAN_Info WHERE Name = ?;", [Name.toString()], function(err, vlans) {
				if (vlans && vlans[0])
				{
					var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
					const iv = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
					var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
					var encrypted = cipher.update('JOIN', 'binary', 'hex');
					encrypted += cipher.update(vlans[0].VLANID, 'hex', 'hex');
					if (vlans[0].CryptoKey)
						encrypted += cipher.update("01", 'hex', 'hex');
					else
						encrypted += cipher.update("00", 'hex', 'hex');
					encrypted += cipher.update(Name+"\0", 'utf8', 'hex');
					encrypted += cipher.final('hex');
					server.send(Buffer.from(encrypted,'hex'), rinfo.port, rinfo.address);
                    console.log(`server got join request for ${user[0].Username} to join '${Name}'`);
				}
				else
				{
					var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
					const iv = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
					var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
					var encrypted = cipher.update('EROR', 'binary', 'hex');
					encrypted += cipher.update('Error: Can not find VLAN \'' + Name + "'\0", 'utf8', 'hex');
					encrypted += cipher.final('hex');
					server.send(Buffer.from(encrypted,'hex'), rinfo.port, rinfo.address);
                    console.log(`server got INVALID join request for ${user[0].Username} to join '${Name}'`);
				}
			});
		}
		else if (user[0].Valid && Type == "RGST")
		{
			var VLAN = Buff.slice(24,44);
            database.query("SELECT VLANID, Name, Description, CryptoKey, IPv4, IPv6 FROM VLAN_Info WHERE VLANID = ?;", [VLAN.toString('hex')], function(err, vlans) {
				if (vlans && vlans[0])
				{
					const iv = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
					var Key = Buffer.alloc(32);
					if (vlans[0].CryptoKey.length)
                        Key = Buffer.from(vlans[0].CryptoKey, "hex");
					try {
						var decipher = crypto.createDecipheriv('aes-256-cbc', Key, iv);
						var Request = decipher.update(Buff.slice(44));
						Request = Buffer.concat([Request, decipher.final()]);
						if (Request.slice(0,7) != "LETMEIN")
							throw "Invalid Decrypt";
						var MAC = Request.slice(7, 13);
						var IPCount = Request.readUInt8(13);
						console.log(`Got ${IPCount} IPs from ${MAC.toString('hex')} FOR ${vlans[0].VLANID}:`);
						for (var x=0; x<IPCount; x++)
						{
							var IPv6 = Request.slice(14+(18*x), 14+(18*x)+2).toString('hex');
							for (var y=2; y<16; y+=2)
								IPv6 += ":" + Request.slice(14+(18*x)+(y), 14+(18*x)+(y)+2).toString('hex')
							var IP = IPAddr.parse(IPv6);
							var Port = Request.readUInt16BE(14+(18*x)+16, 2);
							console.log(IP.toString(), Port);
                            database.query("REPLACE INTO VLAN_Membership (VLANID, UserID, MAC, IP, Port, Time) VALUES (?, ?, ?, ?, ?, ?)", [VLAN.toString('hex'), UserHash.toString('hex'), MAC.toString('hex'), IP.toString(), Port, Math.floor(new Date() / 1000)], function (err, res) { console.log(err, res);});
						}
						var VLANIPv4 = vlans[0].IPv4;
						if (!VLANIPv4)
							VLANIPv4 = "10.0.0.0/8";
						var VLANIPv6 = vlans[0].IPv6;
						if (!VLANIPv6)
							VLANIPv6 = "fd00::/64";
						var IPv4Subnet = IPAddr.createCIDR(VLANIPv4);
						var IPv6Subnet = IPAddr.createCIDR(VLANIPv6);
                        database.query("SELECT IPv4, IPv6 FROM IP_Allocations WHERE VLANID = ? AND MAC = ?", [vlans[0].VLANID, MAC.toString('hex')], function(err, allocation){
                            if (err)
                                console.log(err);
                            database.query("SELECT UserID, MAC, IP, Port FROM VLAN_Membership WHERE VLANID = ?", [vlans[0].VLANID], function (err, peers) {
                                if (err)
                                    console.log(err);
                                console.log("Sending " + peers.length + " VLAN messages", rinfo.port, rinfo.address);
                                for (var x = 0; x < peers.length; x++)
                                {
                                    // Advertise peer to user
                                    var IP = IPAddr.parse(peers[x].IP);
                                    var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
                                    var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
                                    var encrypted = cipher.update(vlans[0].VLANID, 'hex', 'hex');
                                    encrypted += cipher.update(peers[x].UserID, 'hex', 'hex');
                                    encrypted += cipher.update(peers[x].MAC, 'hex', 'hex');
                                    encrypted += cipher.update(IP.toBuffer().toString('hex'), 'hex', 'hex');
                                    var Port = new Buffer(2);
                                    Port.writeUInt16BE(peers[x].Port, 0);
                                    encrypted += cipher.update(Port.toString('hex'), 'hex', 'hex');
                                    encrypted += cipher.final('hex');
                                    server.send(Buffer.concat([Buffer.from('VLAN'), Buffer.from(encrypted, 'hex')]), rinfo.port, rinfo.address);
                                    console.log("Advertising " + IP.toString() + " port " + peers[x].Port + " to " + user[0].UserID);
                                    database.query("SELECT UserHash, CryptoHash FROM Accounts WHERE UserHash = ?", [peers[x].UserID], function (err, peer) {
                                        if (err)
                                            console.log(err);
                                        if (peer && peer.length)
                                        {
                                            var CryptoKey = Buffer.from(peer[0].CryptoHash, "hex");
                                            var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
                                            var encrypted = cipher.update(vlans[0].VLANID, 'hex', 'hex');
                                            encrypted += cipher.update(peer[0].UserHash, 'hex', 'hex');
                                            encrypted += cipher.update(MAC.toString('hex'), 'hex', 'hex');
                                            var UserIP = IPAddr.parse(rinfo.address);
                                            encrypted += cipher.update(UserIP.toBuffer().toString('hex'), 'hex', 'hex');
                                            var Port = new Buffer(2);
                                            Port.writeUInt16BE(rinfo.port, 0);
                                            encrypted += cipher.update(Port.toString('hex'), 'hex', 'hex');
                                            encrypted += cipher.final('hex');
                                            console.log("Advertising " + UserIP.toString() + " port " + rinfo.port + " to " + peer[0].UserHash);
                                        }
                                    });
                                }
							});
							if (allocation.length)
							{
                                database.query("UPDATE IP_Allocations SET Time = ? WHERE VLANID = ? AND MAC = ?", [Math.floor(new Date() / 1000), vlans[0].VLANID, MAC.toString('hex')], function (err) { if (err) { console.log(err); }});
								console.log("Already allocated " + allocation[0].IPv4 + " and " + allocation[0].IPv6);
								var IPv4 = IPAddr.parse(allocation[0].IPv4);
								var IPv6 = IPAddr.parse(allocation[0].IPv6);
                                var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
								var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
								var encrypted = cipher.update(vlans[0].VLANID, 'hex', 'hex');
								encrypted += cipher.update(IPv4.toBuffer().slice(12,16), 'hex', 'hex');
								encrypted += cipher.update(Buffer.from([IPv4Subnet.prefixLength()]).toString('hex'), 'hex', 'hex');
								encrypted += cipher.update(IPv6.toBuffer(), 'hex', 'hex');
								encrypted += cipher.update(Buffer.from([IPv6Subnet.prefixLength()]).toString('hex'), 'hex', 'hex');
								encrypted += cipher.final('hex');
								server.send(Buffer.concat([Buffer.from('RGST'), Buffer.from(encrypted,'hex')]), rinfo.port, rinfo.address);
							}
							else
							{
                                database.query("SELECT MAC, IPv4, IPv6 FROM IP_Allocations WHERE VLANID = ?", [vlans[0].VLANID], function (err, allocations) {
                                    if (err)
                                        console.log(err);
									var IPv4_Taken = function(IP){
										for (var x=0; x<allocations.length; x++)
										{
											if (allocations[x].IPv4 == IP.toString())
												return true;
										}
										return false;
									};
									var IPv6_Taken = function(IP){
										for (var x=0; x<allocations.length; x++)
										{
											if (allocations[x].IPv6 == IP.toString())
												return true;
										}
										return false;
									};

									var IPv4=IPv4Subnet.first();
									while (IPv4_Taken(IPv4))
										IPv4 = IPv4.offset(1);
									var IPv6=IPv6Subnet.first();
									while (IPv6_Taken(IPv6))
										IPv6 = IPv6.offset(1);
                                    database.query("INSERT INTO IP_Allocations (VLANID, MAC, IPv4, IPv6, Time) VALUES (?,?,?,?,?)", [vlans[0].VLANID, MAC.toString('hex'), IPv4.toString(), IPv6.toString(), Math.floor(new Date() / 1000)], function(err){
                                        if (err)
                                            console.log(err);
										console.log("Allocating " + IPv4.toString() + " and " + IPv6.toString());
                                        var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
										var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
										var encrypted = cipher.update(vlans[0].VLANID, 'hex', 'hex');
										encrypted += cipher.update(IPv4.toBuffer().slice(12,16), 'hex', 'hex');
										encrypted += cipher.update(Buffer.from([IPv4Subnet.prefixLength()]).toString('hex'), 'hex', 'hex');
										encrypted += cipher.update(IPv6.toBuffer(), 'hex', 'hex');
										encrypted += cipher.update(Buffer.from([IPv6Subnet.prefixLength()]).toString('hex'), 'hex', 'hex');
										encrypted += cipher.final('hex');
										server.send(Buffer.concat([Buffer.from('RGST'), Buffer.from(encrypted,'hex')]), rinfo.port, rinfo.address);
									});
								});
							}
						});
					} catch(e) {
						console.log(e);
						var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
						var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
						var encrypted = cipher.update('EROR', 'binary', 'hex');
						encrypted += cipher.update('Invalid password for \'' + vlans[0].Name + "\'\0", 'utf8', 'hex');
						encrypted += cipher.final('hex');
						server.send(Buffer.from(encrypted,'hex'), rinfo.port, rinfo.address);
						console.log(`server got invalid RGST from ${user.Username} for '${vlans[0].VLANID}'`);
					}
				}
			});
		}
		else if (user[0].Valid && Type == "PEER")
		{
			var Peer = Buff.slice(24,44);
            database.query("SELECT Username FROM Accounts WHERE UserHash = ?;", [Peer.toString('hex')], function(err, peer) {
				if (!peer.length)
				{
					var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
					const iv = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
					var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
					var encrypted = cipher.update(Peer.toString('hex'), 'hex', 'hex');
					encrypted += cipher.update("Unknown\0", 'utf8', 'hex');
					encrypted += cipher.final('hex');
					server.send(Buffer.concat([Buffer.from('PEER'), Buffer.from(encrypted,'hex')]), rinfo.port, rinfo.address);
					console.log("PEER request for unknown user " + Peer.toString('hex'));
				}
				else
				{
					var CryptoKey = Buffer.from(user[0].CryptoHash, "hex");
					const iv = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
					var cipher = crypto.createCipheriv('aes-256-cbc', CryptoKey, iv);
					var encrypted = cipher.update(Peer.toString('hex'), 'hex', 'hex');
					encrypted += cipher.update(peer[0].Username + "\0", 'utf8', 'hex');
					encrypted += cipher.final('hex');
					server.send(Buffer.concat([Buffer.from('PEER'), Buffer.from(encrypted,'hex')]), rinfo.port, rinfo.address);
					console.log("PEER request for user " + Peer.toString('hex') + " = " + peer[0].Username);
				}
			});
		}
		else
			console.log(`server ignored: ${Type} from ${UserHash.toString('hex')} ${rinfo.address}:${rinfo.port}`);
	});
});

server.bind(55555);
console.log("Node UDP Server UP at " + ServerName);