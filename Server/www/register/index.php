<?php header("Content-Type: text/html; charset=utf-8"); ob_start();	require "../Database.php";?>
<!DOCTYPE html>
<html>
	<head>
		<title>Register MegaLAN Account</title>
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
if ($_SERVER['REQUEST_METHOD'] == "POST")
{
	if (!$_POST['Email'] || !$_POST['Username'])
	{
		echo "<H2 class='error'>Please enter a username and email address.</H2>";
	}
	else if (!strpos($_POST['Email'], '@'))
	{
		echo "<H2 class='error'>Please enter a valid email address.</H2>";
	}
	else if (strlen($_POST['Username']) > 50)
	{
		echo "<H2 class='error'>Username too long.</H2>";
	}
	else
	{
		$db = new Database();
		$UserHash = hash('sha1', $_POST['Username']);
		$row = $db->Query("SELECT COUNT(*) as COUNT FROM Accounts WHERE Username = :username OR UserHash = :userhash", [':username'=>$_POST['Username'], ':userhash'=>$UserHash])->fetch();
		if ($row['COUNT'] == 0)
		{
			$row = $db->Query("SELECT COUNT(*) as COUNT FROM Accounts WHERE Email = :email", [':email'=>$_POST['Email']])->fetch();
			if ($row['COUNT'] < 5)
			{
				$key = bin2hex(openssl_random_pseudo_bytes(8));
				$db->Query("INSERT INTO Accounts (Username, UserHash, Email, VerifyKey, VerifyTime) VALUES(:username, :userhash, :email, :key, :time)", [':username'=>$_POST['Username'], ':userhash'=>$UserHash, ':email'=>$_POST['Email'], ':key'=>$key, ':time'=>time()]);
				echo "<H2>Your account has been created.</H2>";
				echo "<H3>An email has been sent to " . $_POST['Email'] . " with further instructions.</H3>";
				file_get_contents("http://[::1]:8080/API/MAIL/SIGNUP/".$key);
				die("</body></html>");
			}
			else
			{
				echo "<H2 class='error'>You have too many accounts.</H2>";
			}
		}
		else
		{
			echo "<H2 class='error'>Username already registered.</H2>";
		}
	}
}
?>		
		<form method="POST">
			<h1 class="font-weight-normal">MegaLAN</h1>
			<h1 class="h3 font-weight-normal">Register Account</h1>
			<div class="form-label-group">
				<label for="username">Username</label>
				<input name="Username" id="username" type="username" class="form-control" placeholder="Username" required autofocus>
			</div>
			<div class="form-label-group">
				<label for="email">Email address</label>
				<input name="Email" id="email" type="email" class="form-control" placeholder="Email address" required>
			</div>
			<button class="btn btn-lg btn-primary btn-block" type="submit">Sign up</button>
		</form>
	</body>
</html>