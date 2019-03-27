<?php header("Content-Type: text/html; charset=utf-8"); ob_start();	require "../Database.php";
$db = new Database();
$row = $db->Query("SELECT COUNT(*) as Valid, Username FROM Accounts WHERE UserHash = :userhash AND Token = :token", [':userhash'=>$_GET['user'], ':token'=>$_GET['token']])->fetch();
if (!$row || !$row['Valid'])
{
	var_dump($row);
	die("Invalid token.");
}
?>
<!DOCTYPE html>
<html>
	<head>
		<title>Create MegaLAN VLAN</title>
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
function ParseIPv4($IPv4)
{
	$parts = explode("/", $IPv4);
	if (count($parts) == 2 && $parts[1]>=8 && $parts[1]<30 && strlen(@inet_pton($parts[0]))==4)
	{
		return [inet_pton($parts[0]),$parts[1]];
	}
	return false;
}
function ParseIPv6($IPv6)
{
	$parts = explode("/", $IPv6);
	if (count($parts) == 2 && $parts[1]>=8 && $parts[1]<128 && strlen(@inet_pton($parts[0]))==16)
	{
		return [inet_pton($parts[0]),$parts[1]];
	}
	return false;
}
if ($_SERVER['REQUEST_METHOD'] == "POST")
{
	if (!$_POST['Name'] || strlen($_POST['Name'])<2 || strlen($_POST['Name'])>150)
	{
		echo "<h3 class='error'>Invalid Name</h3>";
	}
	else if ($_POST['IPv4'] && !ParseIPv4($_POST['IPv4']))
	{
		echo "<h3 class='error'>Invalid IPv4 Subnet<br>Example: 10.0.0.0/24</h3>";
	}
	else if ($_POST['IPv6'] && !ParseIPv6($_POST['IPv6']))
	{
		echo "<h3 class='error'>Invalid IPv6 Subnet<br>Example: fd00::/64</h3>";
	}
	else
	{
		$rows = $db->Query("SELECT VLANID FROM VLAN_Info WHERE Name = :Name", [':Name'=>$_POST['Name']])->fetchAll();
		if (count($rows))
		{
			echo "<h3 class='error'>Name is already in use.</h3>";
		}
		else
		{
			$VLANID = bin2hex(openssl_random_pseudo_bytes(20));
			$rows = $db->Query("SELECT VLANID FROM VLANs WHERE VLANID = :ID", [':ID'=>$VLANID])->fetchAll();
			if (count($rows))
			{
				echo "<h3 class='error'>An error occurred, try again.</h3>";
			}
			else
			{
				$db->Query("INSERT INTO VLANs (VLANID, Public) VALUES(:ID, :Public)", [
					':ID'=>$VLANID,
					':Public'=>(isset($_POST['Hidden']) && $_POST['Hidden'] == 'on')?0:1
				]);
				$IPv4=isset($_POST['IPv4'])?$_POST['IPv4']:'';
				if (!strlen($IPv4))
					$IPv4="10.0.0.0/8";
				$IPv6=isset($_POST['IPv6'])?$_POST['IPv6']:'';
				if (!strlen($IPv6))
					$IPv6="fd00::/64";
				$db->Query("INSERT INTO VLAN_Info (VLANID, Name, Description, IPv4, IPv6, CryptoKey) VALUES(:ID, :Name, :Description, :IPv4, :IPv6, :CryptoKey)", [
					':ID'=>$VLANID,
					':Name'=>$_POST['Name'],
					':Description'=>$_POST['Description'],
					':IPv4'=>$IPv4,
					':IPv6'=>$IPv6,
					':CryptoKey'=>(isset($_POST['Password']) && strlen($_POST['Password']))?hash('sha256', $_POST['Password']):''
				]);
				die("<h2>VLAN Created. You can now connect your client.</h2></body></html>");
			}
		}
	}
//	die("</body></html>");
}
?>
		<form method="POST">
			<h1 class="font-weight-normal">MegaLAN</h1>
			<h1 class="h3 font-weight-normal">Create VLAN</h1>
			<div class="form-label-group">
				<label for="Name">Name (Required)</label>
				<input name="Name" id="Name" type="text" class="form-control" placeholder="Name" value="<?php if (isset($_POST['Name']))echo $_POST['Name']; else if (isset($_GET['name']))echo $_GET['name'];?>" required autofocus>
			</div>
			<div class="form-label-group">
				<label for="Hidden">
					Hidden
					<input name="Hidden" id="Hidden" type="checkbox" <?php if (isset($_POST['Hidden']) && $_POST['Hidden']=='on') echo "CHECKED";?>>
				</label>
			</div>
			<div class="form-label-group">
				<label for="Description">Description</label>
				<input name="Description" id="Description" type="text" class="form-control" placeholder="Description" value="<?php if (isset($_POST['Description'])) echo $_POST['Description'];?>">
			</div>
			<div class="form-label-group">
				<label for="Password">Password</label>
				<input name="Password" id="Password" type="password" class="form-control" placeholder="Password" value="<?php if (isset($_POST['Password'])) echo $_POST['Password'];?>">
			</div>
			<div class="form-label-group">
				<label for="IPv4">IPv4 Subnet (10.0.0.0/8)</label>
				<input name="IPv4" id="IPv4" type="text" class="form-control" placeholder="(auto)" value="<?php if (isset($_POST['IPv4'])) echo $_POST['IPv4'];?>">
			</div>
			<div class="form-label-group" style="display:none;">
				<label for="IPv6">IPv6 Subnet (fd00::/64)</label>
				<input name="IPv6" id="IPv6" type="text" class="form-control" placeholder="(auto)" value="<?php if (isset($_POST['IPv6'])) echo $_POST['IPv6'];?>">
			</div>
			<button class="btn btn-lg btn-primary btn-block" type="submit">Create VLAN</button>
		</form>
	</body>
</html>