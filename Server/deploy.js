"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
/*
 * Deploy script
 *
 * Used for easily deploying code to multiple servers.
 *
 * Visual Studio project is set to run this script when I click "Start" ;)
 */
var child_process_1 = require("child_process");
var SSHKEY = "c:\\users\\notmi\\onedrive\\desktop\\ssh";
function ExecRemote(server, cmd) {
    try {
        var Output = child_process_1.execSync('ssh -i ' + SSHKEY + ' root@' + server + ' \"' + cmd + '\"');
        console.log(server, cmd, Output.toString());
    }
    catch (E) {
        console.log("Exec error", cmd);
    }
}
function PushRemote(server, local, remote) {
    try {
        var Output = child_process_1.execSync('scp -i ' + SSHKEY + ' -r ' + local + ' root@' + server + ':' + remote);
        console.log(server, local, remote, Output.toString());
    }
    catch (E) {
        console.log("Push error", local, remote);
    }
}
function DoServer(server) {
    //	ExecRemote(server, '/MegaLAN/app.lua clean');
    PushRemote(server, "app.lua *.js www", "/MegaLAN/");
    ExecRemote(server, 'chmod +x /MegaLAN/app.lua /MegaLAN/*.js && /MegaLAN/app.lua restart >/dev/null 2>&1 &');
}
DoServer("uk.megalan.app");
DoServer("la.megalan.app");
DoServer("ch.megalan.app");
//# sourceMappingURL=deploy.js.map