#!/usr/local/sbin/container
require("module/network")
network:AddNameserver("::1")
network:AddNameserver("9.9.9.9")
network:AddNameserver("8.8.8.8")
require("module/caddy")
require("module/php")
php.debug=true
local Hostname = exec("hostname", true)
Hostname = exec("echo -n " .. Hostname, true)
Mount{path='/www', type="map", source="/MegaLAN/www"}
Mount{path='/MegaLAN/', type="map", source="/MegaLAN/"}

function install_container()
	install_package("ca-certificates unbound")
	exec_or_die("wget -O- https://deb.nodesource.com/setup_11.x | bash -")
	install_package("nodejs")
	exec_or_die("npm install mysql")
	exec_or_die("npm install sendmail")
	exec_or_die("npm install ip6addr")
	write_file("etc/Cloudflare", read_file("/etc/Cloudflare"))
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

]] .. Hostname .. [[.megalan.app:443 {
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
		dns cloudflare
	}
	header / Strict-Transport-Security "max-age=31536000;"
}

megalan.app:443 {
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
		dns cloudflare
	}
	header / Strict-Transport-Security "max-age=31536000;"
}
]])
	return 0
end

function background()
	exec("cd /root && caddy -conf /etc/Caddyfile -env /etc/Cloudflare -email notmike@notmike.co.uk -agree &")
	exec("unbound &")
	exec("cd /MegaLAN && while true; do ./UDPServer.js " .. Hostname .. ".megalan.app; sleep 1; done&")
	exec("cd /MegaLAN && while true; do ./HTTPServer.js; sleep 1; done&")
	return 0
end
