#!/usr/bin/nodejs
var http = require('http');
var url = require('url');
var sendmail = require("sendmail")();

var mysql = require('mysql');
var database = mysql.createPool({
	connectionLimit: 20,
	host: 'localhost',
	user: 'MegaLAN',
	password: 'MegaLAN',
	database: 'MegaLAN'
});

var Handler = function (req, res) {
	console.log("Request started");
	req.on('error', function (error) {
		console.log("Request Error", error);
	});
	var URL = url.parse(req.url, true);
	var Path = req.url.split("/");
	console.log("Request " + req.socket.remoteAddress + " " + req.method + " " + req.url);
	try {
		if (Path[1] != "API")
			return res.end("ERROR");
		if (Path[2] == 'MAIL' && Path[3] == "SIGNUP" && Path[4])
		{
			database.query("SELECT Username, Email, VerifyKey FROM Accounts WHERE VerifyKey = ?", [Path[4]], (err, row) => {
				if (err) {
					return console.error(err.message);
				}
				if (row.length)
				{
					console.log("Send Email", row);
					Message = "Hello " + row[0].Username + "!\n<BR/><BR/>\n";
					Message += "Welcome to MegaLAN, you are nearly ready to connect. Just one last thing to do, click the link below and set your password.\n<BR/><BR/>\n";
					Message += "https://MegaLAN.app/password/?" + row[0].VerifyKey + "\n<BR/><BR/>\n";
					Message += "Once that is done, you can start using MegaLAN.\n<BR/><BR/>\n";
					Message += "If it wasn't you that registered this account, please ignore this email.\n\n";
					sendmail({
							to: row[0].Email,
							from: "MegaLAN <No-Reply@MegaLAN.app>",
							subject: "Welcome to MegaLAN",
							html: Message,
						},
						function (err, result) {
							if (err) console.log(err);
							console.log("SENDMAIL RESULT", result);
						});
				}
			});
			res.writeHead(200, {'Content-Type': "text/plain; charset=utf8"});
			return res.end("SENT");
		}
		if (Path[2] == 'MAIL' && Path[3] == "RESET" && Path[4])
		{
			database.query("SELECT Username, Email, VerifyKey FROM Accounts WHERE VerifyKey = ?", [Path[4]], (err, row) => {
				if (err) {
					return console.error(err.message);
				}
				if (row.length)
				{
					console.log("Send Email", row);
					Message = "Hello " + row[0].Username + "!\n<BR/><BR/>\n";
					Message += "To reset your password, click the link below.\n<BR/><BR/>\n";
					Message += "https://MegaLAN.app/password/?" + row[0].VerifyKey + "\n<BR/><BR/>\n";
					Message += "Once that is done, you can log in with your new password.\n<BR/><BR/>\n";
					Message += "If it wasn't you that requested this password reset, please ignore this email.\n<BR/><BR/>\n";
					sendmail({
						to: row[0].Email,
						from: "MegaLAN <No-Reply@MegaLAN.app>",
						subject: "MegaLAN Password Reset",
						html: Message,
					},
						function (err, result) {
							if (err) console.log(err);
							console.log("SENDMAIL RESULT", result);
						});
				}
			});
			res.writeHead(200, {'Content-Type': "text/plain; charset=utf8"});
			return res.end("SENT");
		}
		res.writeHead(500, {'Content-Type': "text/html; charset=utf8"});
		res.end("Content");
	} catch (E) {
		res.writeHead(500, {'Content-Type': "text/plain"});
		res.end(JSON.stringify(E));
		console.log(E);
	}
}
var server = http.createServer(Handler);
server.listen(8080, '::1');
process.on('uncaughtException', function (err) {
	console.error(err);
});
console.log("Node HTTP UP");
