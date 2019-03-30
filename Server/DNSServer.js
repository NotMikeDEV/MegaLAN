#!/usr/bin/nodejs
var dns = require('native-dns');
var args = process.argv.slice(2);
var ServerName = args[0];

var IPv4 = false;
var IPv6 = false;

var DNSHandler = function (request, response) {
	console.log("DNS Request", request);
	var question = request.question[0];
	var hostname = question.name.toLowerCase();
	var Host = hostname.split(".");

	response.answer.push(dns.SOA({
		name: hostname,
		ttl: 300,
		primary: 'MegaLAN.app',
		admin: 'MegaLAN',
		serial: 1,
		refresh: 600,
		retry: 600,
		expiration: 3600,
		minimum: 5,
	}));
	response.authority.push(dns.NS({
		name: hostname,
		data: ServerName,
		ttl: 5,
	}));
	dns.resolve(ServerName, 'A', function (err, IPv4) {
		dns.resolve(ServerName, 'AAAA', function (err, IPv6) {
			if (IPv4.length) {
				for (var x in IPv4) {
					response.answer.push(dns.A({
						name: request.question[0].name,
						address: IPv4[x],
						ttl: 5,
					}));
				}
			}
			if (IPv6.length) {
				for (var x in IPv6) {
					response.answer.push(dns.AAAA({
						name: request.question[0].name,
						address: IPv6[x],
						ttl: 5,
					}));
				}
			}
			response.send();
		});
	});
};
var ErrorHandler = function (err, buff, req, res) {
	console.log(err.stack);
};
var UDP4 = dns.createServer({ dgram_type: 'udp4' }).on('request', DNSHandler).on('error', ErrorHandler);
var TCP4 = dns.createTCPServer({ dgram_type: 'tcp4' }).on('request', DNSHandler).on('error', ErrorHandler);
var UDP6 = dns.createServer({ dgram_type: 'udp6' }).on('request', DNSHandler).on('error', ErrorHandler);
var TCP6 = dns.createTCPServer({ dgram_type: 'tcp6' }).on('request', DNSHandler).on('error', ErrorHandler);
dns.resolve(ServerName, 'A', function (err, IPv4) {
	for (var x in IPv4)
		console.log("DNS Server UDP", IPv4[x], UDP4.serve(53, IPv4[x]));
});
dns.resolve(ServerName, 'A', function (err, IPv4) {
	for (var x in IPv4)
		console.log("DNS Server TCP", IPv4[x], TCP4.serve(53, IPv4[x]));
});
dns.resolve(ServerName, 'AAAA', function (err, IPv6) {
	for (var x in IPv6)
		console.log("DNS Server UDP6", IPv6[x], UDP6.serve(53, IPv6[x]));
});
dns.resolve(ServerName, 'AAAA', function (err, IPv6) {
	for (var x in IPv6)
		console.log("DNS Server TCP6", IPv6[x], TCP6.serve(53, IPv6[x]));
});
