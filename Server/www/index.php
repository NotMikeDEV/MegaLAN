<?php header("Content-Type: text/html; charset=utf-8"); ob_start();	require "Database.php";?>
<!DOCTYPE html>
<html lang="en">
	<head>
		<title>MegaLAN</title>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<meta name="theme-color" id="theme" content="#003399">
		<link rel="shortcut icon" type="image/ico" href="/favicon.ico">
		<link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css">
		<script src="https://code.jquery.com/jquery-3.3.1.min.js" crossorigin="anonymous"></script>
		<script src="https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/js/bootstrap.min.js" crossorigin="anonymous"></script>
		<script src="https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js" crossorigin="anonymous"></script>
		<link rel="stylesheet" href="/StyleSheet.css">
	</head>
	<body class="text-center">
		<div class="cover-container d-flex h-100 p-3 mx-auto flex-column">
			<header class="masthead mb-auto">
				<div class="inner">
					<h3 class="masthead-brand">MegaLAN</h3>
					<nav class="nav nav-masthead justify-content-center">
						<a class="nav-link active" href="#">Home</a>
						<a class="nav-link" href="#">Account Management</a>
					</nav>
				</div>
			</header>
			<main role="main" class="inner cover">
				<p></p>
				<p class="lead">MegaLAN is an easy to use free p2p VPN client.</p>
				<p class="lead">Just install, log in, select a network, and connect!</p>
				<p>/!\</p>
				<p><A HREF="https://github.com/NotMikeDEV/MegaLAN/blob/master/MegaLAN-64bit.exe?raw=true" class="btn btn-lg btn-secondary">Download 64bit Installer</A></p>
				<p><A HREF="https://github.com/NotMikeDEV/MegaLAN/blob/master/MegaLAN-32bit.exe?raw=true" class="btn btn-lg btn-secondary">Download 32bit Installer</A></p>
				<p><A HREF="https://github.com/NotMikeDEV/MegaLAN" class="">Source Code</A></p>
			</main>

			<footer class="mastfoot mt-auto">
				<div class="inner">
					<p>MegaLAN is a project currently in development by <a href="https://notmike.dev/">NotMike</a>.</p>
				</div>
			</footer>
		</div>
	</BODY>
</HTML>