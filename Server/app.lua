#!/usr/local/sbin/container
require("module/network")
network:AddNameserver("::1")
network:AddNameserver("9.9.9.9")
network:AddNameserver("8.8.8.8")
require("module/caddy")
require("module/php")
php.debug=true
local WS = caddy:AddWebsite{hostname='', port="80", root='/www/redirect'}
local WS = caddy:AddWebsite{hostname='www.megalan.app', port="80", root='/www/redirect'}
local WS = caddy:AddWebsite{hostname='megalan.app', port="80", root='/www'}
local Hostname = exec("hostname", true)
Hostname = exec("echo -n " .. Hostname, true)
local Server = caddy:AddWebsite{hostname=Hostname .. '.megalan.app', port="443", root='/www'}
Mount{path='/www', type="map", source="/MegaLAN/www"}
Mount{path='/MegaLAN/', type="map", source="/MegaLAN/"}

function install_container()
	install_package("ca-certificates unbound")
	exec_or_die("wget -O- https://deb.nodesource.com/setup_8.x | bash -")
	install_package("nodejs")
	exec_or_die("npm install mysql")
	exec_or_die("npm install mailer")
	exec_or_die("npm install ip6addr")
	return 0
end

function apply_config()
	write_file("/etc/unbound/unbound.conf", [[
server:
		interface: 127.0.0.1
		interface: ::1
		access-control: 127.0.0.1/32 allow
		access-control: ::1/128 allow
]])
	return 0
end

function background()
	exec("unbound")
	exec("cd /MegaLAN && while true; do ./UDPServer.js " .. Hostname .. ".megalan.app; sleep 1; done&")
	exec("cd /MegaLAN && while true; do ./HTTPServer.js; sleep 1; done&")
	return 0
end
