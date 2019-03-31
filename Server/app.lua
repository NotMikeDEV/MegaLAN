#!/usr/local/sbin/container
require("module/network")
network:AddNameserver("::1")
network:AddNameserver("9.9.9.9")
network:AddNameserver("8.8.8.8")
require("module/caddy")
require("module/php")

local ServerName = exec("hostname -s", true)
ServerName = exec("echo -n " .. ServerName, true)
local DomainName = exec("hostname -d", true)
DomainName = exec("echo -n " .. DomainName, true)
local IPv4 = exec([[ip addr|grep "inet "|grep global|cut -d " " -f 6|cut -d "/" -f 1|head -n 1]], true)
IPv4 = exec("echo -n " .. IPv4, true)
local IPv6 = exec([[ip addr|grep "inet6 "|grep global|cut -d " " -f 6|cut -d "/" -f 1|head -n 1]], true)
IPv6 = exec("echo -n " .. IPv6, true)

Mount{path='/www', type="map", source="/MegaLAN/www"}
Mount{path='/MegaLAN/', type="map", source="/MegaLAN/"}

function install_container()
	install_package("ca-certificates unbound")
	exec_or_die("wget -O- https://deb.nodesource.com/setup_11.x | bash -")
	install_package("nodejs")
	exec_or_die("npm install mysql")
	exec_or_die("npm install sendmail")
	exec_or_die("npm install ip6addr")
	exec_or_die("npm install native-dns")
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
	write_file("/etc/Caddyfile", [[
:80 {
	root /www/redirect
	fastcgi / /run/php/php7.0-fpm.sock {
			ext     .php
			split   .php
			index   index.php
	}
}

]] .. ServerName .. "." .. DomainName .. [[:443 {
	root /www
	fastcgi / /run/php/php7.0-fpm.sock {
			ext     .php
			split   .php
			index   index.php
	}
	log /var/log/caddy.log {
			rotate_size 5
			rotate_age  14
			rotate_keep 20
	}
	tls {
		dns rfc2136
	}
	header / Strict-Transport-Security "max-age=31536000;"
}

]] .. DomainName .. [[:443 {
	root /www
	fastcgi / /run/php/php7.0-fpm.sock {
		ext     .php
		split   .php
		index   index.php
	}
	log /var/log/caddy.log {
		rotate_size 5
		rotate_age  14
		rotate_keep 20
	}
	tls {
		dns rfc2136
	}
	header / Strict-Transport-Security "max-age=31536000;"
}
]])
	return 0
end

function background()
	exec("unbound &")
	exec("cd /MegaLAN && while true; do ./UDPServer.js " .. ServerName .. "." .. DomainName .. "; sleep 1; done&")
	exec("cd /MegaLAN && while true; do ./DNSServer.js " .. ServerName .. " " .. DomainName .. " " .. IPv4 .. " " .. IPv6 .. "; sleep 1; done&")
	exec("export RFC2136_NAMESERVER=127.0.0.1:99; sleep 2; cd /root && while true; do caddy -conf /etc/Caddyfile -email notmike@notmike.co.uk -agree; sleep 1; done&")
	exec("cd /MegaLAN && while true; do ./HTTPServer.js; sleep 1; done&")
	return 0
end
