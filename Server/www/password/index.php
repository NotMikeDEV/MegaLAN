<?php header("Content-Type: text/html; charset=utf-8"); ob_start();	require "../Database.php";?>
<!DOCTYPE html>
<html>
	<head>
		<title>MegaLAN Password</title>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<meta name="theme-color" id="theme" content="#003399">
		<link rel="shortcut icon" type="image/ico" href="/favicon.ico">
		<link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css">
		<style>
		@font-face {
			font-family: 'opendyslexic3';
			src: url('https://antijingoist.github.io/web-accessibility/fonts/opendyslexic3-regular.ttf') format('truetype');
			font-weight: normal;
			font-style: normal;
		}
		html, body {
		  height: 100%;
		}
		body {
			font-family: 'OpenDyslexic3';
			align-items: center;
			padding-top: 40px;
			padding-bottom: 40px;
			background-color: #f5f5f5;
		}
		h1, h2, h3 {
			text-align: center;
			width: 100%;
			padding: 15px;
			margin: auto;
		}
		.error {
			color: #F00;
		}
		form {
			width: 100%;
			max-width: 330px;
			padding: 15px;
			margin: auto;
		}
		form input:focus {
			z-index: 2;
		}
		form input {
			margin-bottom: 10px;
			border-top-left-radius: 5;
			border-top-right-radius: 5;
			border-color: silver;
		}
		
		</style>
		<script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js" type="text/javascript"></script>
	</head>
	<body>
<?php
if ($_SERVER['QUERY_STRING'])
{
	$db = new Database();
	$row = $db->Query("SELECT * FROM Accounts WHERE VerifyKey = :key", [':key'=>$_SERVER['QUERY_STRING']])->fetch();
	if ($row)
	{
		if ($_SERVER['REQUEST_METHOD'] == "POST" && $_POST['Password1'] && $_POST['Password2'])
		{
			if ($_POST['Password1'] == $_POST['Password2'])
			{
				$PasswordHash = hash('sha1', $row['Username'].$_POST['Password1']);
				$CryptoHash = hash('sha256', $row['Username'].$_POST['Password1']);
				$row = $db->Query("UPDATE Accounts SET CryptoHash=:CryptoHash, PasswordHash=:PasswordHash, VerifyKey='' WHERE VerifyKey = :key", [':key'=>$_SERVER['QUERY_STRING'], ':PasswordHash'=>$PasswordHash, ':CryptoHash'=>$CryptoHash]);
				echo "<H2>Password set.</H2>";
				echo "<H3>You may now log in with your new password.</H3>";
				die("</body></html>");
			}
			else
			{
				echo "<H2 class='error'>Passwords do not match.</H2>";
			}
		}
?>
		<form method="POST">
			<h1 class="font-weight-normal">MegaLAN</h1>
			<h1 class="h3 font-weight-normal">Reset Password</h1>
			<div class="form-label-group">
				<label for="Password1">Password</label>
				<input name="Password1" id="Password1" type="password" class="form-control" placeholder="Password" required autofocus>
			</div>
			<div class="form-label-group">
				<label for="Password2">Confirm password.</label>
				<input name="Password2" id="Password2" type="password" class="form-control" placeholder="Confirm password" required>
			</div>
			<button class="btn btn-lg btn-primary btn-block" type="submit">Set Password</button>
		</form>
<?php
		die("</body></html>");
	}
	else
	{
		header("Location: /password/");
	}
}
else if ($_SERVER['REQUEST_METHOD'] == "POST")
{
	if (!$_POST['Email'] || !$_POST['Username'])
	{
		echo "<H2 class='error'>Please enter a username and email address.</H2>";
	}
	else
	{
		$db = new Database();
		$key = bin2hex(openssl_random_pseudo_bytes(8));
		$row = $db->Query("UPDATE Accounts SET VerifyKey = :key WHERE Username=:username AND Email=:email", [':key'=>$key, ':username'=>$_POST['Username'], ':email'=>$_POST['Email']]);
		echo "<H3>If your username and password match, you will receive an email shortly.</H3>";
		file_get_contents("http://[::1]:8080/API/MAIL/RESET/".$key);
		die("</body></html>");
	}
}
else
{
?>
		<form method="POST">
			<h1 class="font-weight-normal">MegaLAN</h1>
			<h1 class="h3 font-weight-normal">Reset Password</h1>
			<label for="Username">Username</label>
			<input name="Username" id="Username" type="username" class="form-control" placeholder="Username" required autofocus>
			<label for="Email">Email address</label>
			<input name="Email" id="Email" type="email" class="form-control" placeholder="Email address" required>
			<button class="btn btn-lg btn-primary btn-block" type="submit">Send Email</button>
		</form>
<?php
}?>
	</body>
</html>