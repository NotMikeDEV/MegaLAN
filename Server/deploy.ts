﻿/*
 * Deploy script
 * 
 * Used for easily deploying code to multiple servers.
 * 
 * Visual Studio project is set to run this script when I click "Start" ;)
 */
import { execSync } from "child_process";

var SSHKEY = "c:\\users\\notmi\\onedrive\\desktop\\ssh";
function ExecRemote(server, cmd) {
    try {
        var Output = execSync('ssh -i ' + SSHKEY + ' root@' + server + ' \"' + cmd + '\"');
        console.log(server, cmd, Output.toString());
    } catch (E) {
        console.log("Exec error", cmd);
    }
}
function PushRemote(server, local, remote) {
    try {
        var Output = execSync('scp -i ' + SSHKEY + ' -r ' + local + ' root@' + server + ':' + remote);
        console.log(local, remote, Output.toString());
    } catch (E) {
        console.log("Push error", local, remote);
    }
}
function DoServer(server) {
    PushRemote(server, "app.lua HTTPServer.js UDPServer.js www", "/MegaLAN/");
    ExecRemote(server, 'chmod +x /MegaLAN/app.lua /MegaLAN/*.js && /MegaLAN/app.lua restart >/dev/null 2>&1 &');
}
DoServer("it.megalan.app");
DoServer("la.megalan.app");
DoServer("ch.megalan.app");
