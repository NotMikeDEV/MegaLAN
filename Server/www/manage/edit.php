<?php
$Page = "manage";
$Title = "VLAN Management";
include("../Header.php");
include("../Database.php");
$db = new Database();
if (!isset($_SESSION['User']))
{
	header("Location: .");
	die();
}
if (isset($_GET['id']) && !$_SESSION['User']['IsAdmin'])
{
	$Check = $db->Query("SELECT VLANID FROM VLANs WHERE VLANID = :VLANID AND Owner = :UserHash", [':VLANID'=>$_GET['id'], ':UserHash'=>$_SESSION['User']['UserHash']])->fetchAll();
	if (!count($Check) || $Check[0]['VLANID'] != $_GET['id'])
	{
		header("Refresh: 3;URL=.");
		echo '<h3 class="error">Unable to find VLAN.</h3>';
		include("../Footer.php");
	}
}
if (isset($_GET['id']) && isset($_GET['action']) && $_GET['action'] == "delete")
{
	$Check = $db->Query("BEGIN");
	$Check = $db->Query("DELETE FROM IP_Allocations WHERE VLANID = :VLANID", [':VLANID'=>$_GET['id']]);
	$Check = $db->Query("DELETE FROM VLAN_Membership WHERE VLANID = :VLANID", [':VLANID'=>$_GET['id']]);
	$Check = $db->Query("DELETE FROM VLAN_Info WHERE VLANID = :VLANID", [':VLANID'=>$_GET['id']]);
	$Check = $db->Query("DELETE FROM VLANs WHERE VLANID = :VLANID", [':VLANID'=>$_GET['id']]);
	$Check = $db->Query("COMMIT");
	
	header("Location: .");
	include("../Footer.php");
}
if (isset($_GET['id']) && isset($_GET['action']) && $_GET['action'] == "deleteMAC" && isset($_GET['MAC']))
{
	$Check = $db->Query("DELETE FROM IP_Allocations WHERE VLANID = :VLANID AND MAC = :MAC", [':VLANID'=>$_GET['id'], ':MAC'=>$_GET['MAC']]);
	header("Location: ?id=" . $_GET['id']);
	include("../Footer.php");
}
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
	else if (isset($_POST['IPv4']) && $_POST['IPv4'] && !ParseIPv4($_POST['IPv4']))
	{
		echo "<h3 class='error'>Invalid IPv4 Subnet<br>Example: 10.0.0.0/24</h3>";
	}
	else if (isset($_POST['IPv6']) && $_POST['IPv6'] && !ParseIPv6($_POST['IPv6']))
	{
		echo "<h3 class='error'>Invalid IPv6 Subnet<br>Example: fd00::/64</h3>";
	}
	else
	{
		$rows = $db->Query("SELECT VLANID FROM VLAN_Info WHERE Name = :Name", [':Name'=>$_POST['Name']])->fetchAll();
		if (count($rows) && $rows[0]['VLANID'] != $_GET['id'])
		{
			echo "<h3 class='error'>Name is already in use.</h3>";
		}
		else
		{
			$VLANID = bin2hex(openssl_random_pseudo_bytes(20));
			if (isset($_GET['id']))
				$VLANID = $_GET['id'];
			$rows = $db->Query("SELECT VLANID, Owner FROM VLANs WHERE VLANID = :ID", [':ID'=>$VLANID])->fetchAll();
			if (count($rows))
			{
				$OldKey = $db->Query("SELECT CryptoKey FROM VLAN_Info WHERE VLANID = :ID", [':ID'=>$VLANID])->fetch();
				$db->Query("UPDATE VLANs SET Public = :Public WHERE VLANID = :ID", [
					':ID'=>$VLANID,
					':Public'=>(isset($_POST['Hidden']) && $_POST['Hidden'] == 'on')?0:1
				]);
				$IPv4=isset($_POST['IPv4'])?$_POST['IPv4']:'';
				if (!strlen($IPv4))
					$IPv4="10.0.0.0/8";
				$IPv6=isset($_POST['IPv6'])?$_POST['IPv6']:'';
				if (!strlen($IPv6))
					$IPv6="fd00::/64";
				$CryptoKey = (isset($_POST['Password']) && strlen($_POST['Password']))?hash('sha256', $_POST['Password']):'';
				if ($_POST['Password'] == $OldKey['CryptoKey'])
					$CryptoKey = $OldKey['CryptoKey'];
				$db->Query("UPDATE VLAN_Info SET Name = :Name, Description = :Description, IPv4 = :IPv4, IPv6 = :IPv6, CryptoKey = :CryptoKey WHERE VLANID = :ID", [
					':ID'=>$VLANID,
					':Name'=>$_POST['Name'],
					':Description'=>$_POST['Description'],
					':IPv4'=>$IPv4,
					':IPv6'=>$IPv6,
					':CryptoKey'=>$CryptoKey
				]);
				header("Location: .");
				include("../Footer.php");
			}
			else
			{
				$db->Query("INSERT INTO VLANs (VLANID, Public, Owner) VALUES(:ID, :Public, :Owner)", [
					':ID'=>$VLANID,
					':Public'=>(isset($_POST['Hidden']) && $_POST['Hidden'] == 'on')?0:1,
					':Owner'=>$_SESSION['User']['UserHash']
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
				header("Location: .");
				include("../Footer.php");
			}
		}
	}
}
if (isset($_GET['id']))
{
	echo '<table class="table table-dark">';
	echo '<thead class="thead-light"><tr>';
	echo '<th>MAC</th>';
	echo '<th>IPv4</th>';
	echo '<th>Age</th>';
	echo '<th>Delete</th>';
	echo '</tr></thead>';

	$rows = $db->Query("SELECT MAC, IPv4, Time FROM IP_Allocations WHERE VLANID = :VLANID", [':VLANID'=>$_GET['id']]);
	while ($row = $rows->fetch())
	{
		echo '<tr>';
		echo '<td>' . $row['MAC'] . '</td>';
		echo '<td>' . $row['IPv4'] . '</td>';
		echo '<td>' . gmdate("H:i:s", time() - $row['Time']) . '</td>';
		if (time() - $row['Time'] > 600)
			echo '<td class="text-center"><img src="/icons/delete.png" height="25" onclick="location.href=\'edit.php?id=' . $_GET['id'] . '&action=deleteMAC&MAC=' . $row['MAC'] . '\';" /></td>';
		else
			echo '<td></td>';
		echo '</tr>';
	}
	echo '</table>';
}
?>
<form method="POST">
	<?php
	$VLAN = [
		'Name'=>'',
		'Hidden'=>'',
		'Description'=>'',
		'Password'=>'',
		'IPv4'=>'',
		];
	if (isset($_GET['id']))
	{
		echo '<h1 class="h3 font-weight-normal">Edit VLAN</h1>';
		$VLAN = $db->Query("SELECT Name, Description, CryptoKey, IPv4 FROM VLAN_Info WHERE VLANID = :VLANID", [':VLANID'=>$_GET['id']])->fetch();
		$Desc = $db->Query("SELECT Public FROM VLANs WHERE VLANID = :VLANID", [':VLANID'=>$_GET['id']])->fetch();
		if (!$Desc['Public'])
			$VLAN['Hidden'] = 'on';
		else
			$VLAN['Hidden'] = 'off';
		if ($VLAN['CryptoKey'])
			$VLAN['Password'] = $VLAN['CryptoKey'];
		else
			$VLAN['Password'] = '';
	}
	else
		echo '<h1 class="h3 font-weight-normal">Create VLAN</h1>';
	if (isset($_POST['Name']))
		$VLAN['Name'] = $_POST['Name'];
	if (isset($_POST['Hidden']))
		$VLAN['Hidden'] = $_POST['Hidden'];
	if (isset($_POST['Description']))
		$VLAN['Description'] = $_POST['Description'];
	if (isset($_POST['Password']))
		$VLAN['Password'] = $_POST['Password'];
	if (isset($_POST['IPv4']))
		$VLAN['IPv4'] = $_POST['IPv4'];
?>
	<div class="form-label-group">
		<label for="Name">Name (Required)</label>
		<input name="Name" id="Name" type="text" class="form-control" placeholder="Name" value="<?php echo $VLAN['Name'];?>" required autofocus>
	</div>
	<div class="form-label-group">
		<label for="Hidden">
			Hidden
			<input name="Hidden" id="Hidden" type="checkbox" <?php if ($VLAN['Hidden'] == 'on') echo "CHECKED";?>>
		</label>
	</div>
	<div class="form-label-group">
		<label for="Description">Description</label>
		<input name="Description" id="Description" type="text" class="form-control" placeholder="Description" value="<?php echo $VLAN['Description'];?>">
	</div>
	<div class="form-label-group">
		<label for="Password">Password</label>
		<input name="Password" id="Password" type="password" class="form-control" placeholder="Password" value="<?php echo $VLAN['Password'];?>">
	</div>
	<div class="form-label-group">
		<label for="IPv4">IPv4 Subnet (10.0.0.0/8)</label>
		<input name="IPv4" id="IPv4" type="text" class="form-control" placeholder="(auto)" value="<?php echo $VLAN['IPv4'];?>">
	</div>
	<button class="btn btn-lg btn-primary btn-block" type="submit">Save</button>
</form>
<?php include("../Footer.php");