<?php
header("Content-Type: text/html; charset=utf-8");
ob_start();
error_reporting(E_ALL);
session_start();
?>
<!DOCTYPE html>
<html lang="en">
	<head>
		<title>MegaLAN<?php if (isset($Title)) echo " - " . $Title;?></title>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<meta name="theme-color" id="theme" content="#003399">
		<link rel="shortcut icon" type="image/ico" href="/favicon.ico">
		<link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css" crossorigin="anonymous">
		<link rel="stylesheet" href="/StyleSheet.css">
		<script src="https://code.jquery.com/jquery-3.3.1.min.js" crossorigin="anonymous"></script>
		<script src="https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/js/bootstrap.min.js" crossorigin="anonymous"></script>
	</head>
	<body class="text-center">
			<div class="cover-container d-flex w-100 h-100 p-3 mx-auto flex-column">
				<header class="masthead mb-auto">
					<div class="inner">
						<h3 class="masthead-brand">MegaLAN</h3>
						<nav class="nav nav-masthead justify-content-center">
							<a class="nav-link<?php if ($Page == "home") echo " active"?>" href="/">Home</a>
							<a class="nav-link<?php if ($Page == "manage") echo " active"?>" href="/manage/">Management</a>
							<?php if (isset($_SESSION['User'])) { ?>
							<a class="nav-link" href="/manage/?LogOut">Log Out</a>
							<?php } ?>
						</nav>
					</div>
				</header>
				<main role="main" class="inner cover">
