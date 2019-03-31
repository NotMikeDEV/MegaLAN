#!/usr/bin/nodejs
var dns = require('native-dns');
const IPAddr = require('ip6addr');
var args = process.argv.slice(2);
var ServerName = args[0];
var DomainName = args[1];

var IPv4 = args[2];
var IPv6 = args[3];

var mysql = require('mysql');
var database = mysql.createPool({
	connectionLimit: 5,
	host: 'localhost',
	user: 'MegaLAN',
	password: 'MegaLAN',
	database: 'MegaLAN'
});

setInterval(function () {
	if (IPv4 && IPAddr.parse(IPv4))
		database.query("REPLACE INTO Servers (ServerName, IP, HeartBeatTime) VALUES (?, ?, ?)", [ServerName, IPv4, Math.floor(new Date())], function (err) { if (err) console.log(err); });
	if (IPv6 && IPAddr.parse(IPv6))
		database.query("REPLACE INTO Servers (ServerName, IP, HeartBeatTime) VALUES (?, ?, ?)", [ServerName, IPv6, Math.floor(new Date())], function (err) { if (err) console.log(err); });
	database.query("DELETE FROM Servers WHERE HeartBeatTime < ?", [Math.floor(new Date()) - 90000], function (err) { if (err) console.log(err); });
	database.query("DELETE FROM DNS WHERE Expire < ?", [Math.floor(new Date())], function (err) { if (err) console.log(err); });
}, 60000);
database.query("REPLACE INTO Servers (ServerName, IP, HeartBeatTime) VALUES (?, ?, ?)", [ServerName, IPv4, Math.floor(new Date())], function (err) { if (err) console.log(err); });
database.query("REPLACE INTO Servers (ServerName, IP, HeartBeatTime) VALUES (?, ?, ?)", [ServerName, IPv6, Math.floor(new Date())], function (err) { if (err) console.log(err); });
database.query("DELETE FROM DNS WHERE Expire < ?", [Math.floor(new Date())], function (err) { if (err) console.log(err); });

var DNSHandler = function (request, response) {
	var question = request.question[0];
	var hostname = question.name.toLowerCase();
	var Host = hostname.split(".");
	console.log("DNS Request", Host, question.type, request.address.address);
	response.header.aa = true;
	if (hostname == DomainName || question.type == 6)
	{
		response.answer.push(dns.SOA({
			name: hostname,
			ttl: 300,
			primary: ServerName + "." + DomainName,
			admin: 'MegaLAN',
			serial: 1,
			refresh: 600,
			retry: 600,
			expiration: 3600,
			minimum: 5,
		}));
	}
	if (Host[0] == "_acme-challenge" && question.type == 16) {
		database.query("SELECT Hostname, Type, Value, Expire FROM DNS WHERE Hostname = ? AND Type = 16", [hostname], function (err, Data) {
			if (err) console.log(err);
			for (x in Data)
				response.answer.push(dns.TXT({
					name: hostname,
					data: [Data[x].Value],
					ttl: 3,
				}));
			response.send();
		});
	}
	else if (question.type == 1 || question.type == 28 || question.type == 2 || question.type == 255) {
		database.query("SELECT ServerName, IP FROM Servers WHERE ServerName = ? ORDER BY RAND()", [Host[0]], function (err, result) {
			if (result.length) {
				for (var x in result) {
					var IP = IPAddr.parse(result[x].IP);
					if (IP.kind() == 'ipv4')
						response.answer.push(dns.A({
							name: hostname,
							address: IP.toString(),
							ttl: 300,
						}));
					if (IP.kind() == 'ipv6')
						response.answer.push(dns.AAAA({
							name: hostname,
							address: IP.toString(),
							ttl: 300,
						}));
				}
				response.send();
			}
			else {
				database.query("SELECT ServerName, IP FROM Servers", function (err, result) {
					for (var x in result) {
						var IP = IPAddr.parse(result[x].IP);
						if (IP.kind() == 'ipv4' && (question.type == 1 || question.type == 255)) {
							response.answer.push(dns.A({
								name: hostname,
								address: IP.toString(),
								ttl: 30,
							}));
							response.additional.push(dns.A({
								name: result[x].ServerName + "." + DomainName,
								address: IP.toString(),
								ttl: 300,
							}));
						}
						if (IP.kind() == 'ipv6' && (question.type == 28 || question.type == 255)) {
							response.answer.push(dns.AAAA({
								name: hostname,
								address: IP.toString(),
								ttl: 30,
							}));
							response.additional.push(dns.AAAA({
								name: result[x].ServerName + "." + DomainName,
								address: IP.toString(),
								ttl: 300,
							}));
						}
						if (question.type == 2)
							response.answer.push(dns.NS({
								name: hostname,
								data: result[x].ServerName + "." + DomainName,
								ttl: 300,
							}));
						else
							response.authority.push(dns.NS({
								name: hostname,
								data: result[x].ServerName + "." + DomainName,
								ttl: 300,
							}));
					}
					response.send();
				});
			}
		});
	}
	else
	{
		response.send();
	}
};
var ErrorHandler = function (err, buff, req, res) {
	console.log(err.stack);
};
var UDP4 = dns.createServer({ dgram_type: 'udp4' }).on('request', DNSHandler).on('error', ErrorHandler);
var TCP4 = dns.createTCPServer({ dgram_type: 'tcp4' }).on('request', DNSHandler).on('error', ErrorHandler);
var UDP6 = dns.createServer({ dgram_type: 'udp6' }).on('request', DNSHandler).on('error', ErrorHandler);
var TCP6 = dns.createTCPServer({ dgram_type: 'tcp6' }).on('request', DNSHandler).on('error', ErrorHandler);
console.log("DNS Server UDP", IPv4, UDP4.serve(53, IPv4));
console.log("DNS Server TCP", IPv4, TCP4.serve(53, IPv4));
console.log("DNS Server UDP6", IPv6, UDP6.serve(53, IPv6));
console.log("DNS Server TCP6", IPv6, TCP6.serve(53, IPv6));

var RFC2136Handler = function (request, response) {
	var question = request.question[0];
	var hostname = question.name.toLowerCase();
	console.log("RFC2136", question);
	response.answer.push(dns.SOA({
		name: hostname,
		ttl: 300,
		primary: 'MegaLAN',
		admin: 'MegaLAN',
		serial: 1,
		refresh: 600,
		retry: 600,
		expiration: 3600,
		minimum: 5,
	}));
	if (request.authority && request.authority.length)
	{
		for (var x in request.authority)
		{
			if (request.authority[x].ttl && request.authority[x].class == 1)
			{
				database.query("REPLACE INTO DNS (Hostname, Type, Value, Expire) VALUES (?, ?, ?, ?)", [request.authority[x].name, request.authority[x].type, request.authority[x].data, Math.floor(new Date()) + (request.authority[x].ttl * 1000)], function (err) {
					if (err)
						console.log(err);
					else
						response.send();
				});
			}
		}
	}
};
var RFC2136UDP = dns.createUDPServer({ dgram_type: 'udp4' }).on('request', RFC2136Handler).on('error', ErrorHandler);
console.log("RFC2136 UDP", RFC2136UDP.serve(99, "127.0.0.1"));
var RFC2136TCP = dns.createTCPServer({ dgram_type: 'tcp4' }).on('request', RFC2136Handler).on('error', ErrorHandler);
console.log("RFC2136 TCP", RFC2136TCP.serve(99, "127.0.0.1"));
