<?php
$Page = "manage";
$Title = "VLAN Management";
include("../Header.php");
include("../Database.php");
if ($_SERVER['QUERY_STRING'] == "LogOut")
{
	session_destroy();
	header("Location: ?LoggedOut");
	die();
}
if (!isset($_SESSION['User']))
{
	if (isset($_POST['Username']) && isset($_POST['Password']))
	{
		$PasswordHash = sha1($_POST['Username'].$_POST['Password'], false);
		$db = new Database();
		$user = $db->Query("SELECT Username, UserHash, Email, IsAdmin FROM Accounts WHERE Username = :Username AND PasswordHash = :PasswordHash", [':Username'=>$_POST['Username'], ':PasswordHash'=>$PasswordHash])->fetch();
		if (!$user)
		{
			echo '<h3 class="error">Invalid Username/Password</h3>';
		}
		else
		{
			$_SESSION['User'] = $user;
			header("Location: ?LoggedIn");
		}
	}
	if ($_SERVER['QUERY_STRING'] == "LoggedOut")
		echo '<h3>You have been logged out.</h3>';
?>
<form method="POST">
	<h1 class="font-weight-normal">Log In</h1>
	<div class="form-label-group">
		<label for="Username">Username</label>
		<input name="Username" id="Username" type="text" class="form-control" placeholder="Username" required autofocus>
	</div>
	<div class="form-label-group">
		<label for="Password">Password.</label>
		<input name="Password" id="Password" type="password" class="form-control" placeholder="Password" required>
	</div>
	<button class="btn btn-lg btn-primary btn-block" type="submit">Log In</button>
</form>
<?php
}
else
{
	$db = new Database();
	$VLANs = $db->Query("SELECT VLANID FROM VLANs WHERE Owner = :UserHash ORDER BY ROWID", [':UserHash'=>$_SESSION['User']['UserHash']])->fetchAll();
	if ($_SESSION['User']['IsAdmin'])
		$VLANs = $db->Query("SELECT VLANID FROM VLANs ORDER BY ROWID")->fetchAll();

	if(count($VLANs))
	{
		echo '<table class="table table-dark">';
		echo '<thead class="thead-light"><tr>';
		echo '<th>ID</th>';
		echo '<th>Name</th>';
		echo '<th>Description</th>';
		echo '<th>Manage</th>';
		echo '<th>Delete</th>';
		echo '</tr></thead>';
		foreach($VLANs as $VLAN)
		{
			$VLAN_Info = $db->Query("SELECT VLANID, Name, Description FROM VLAN_Info WHERE VLANID = :VLANID", [':VLANID'=>$VLAN['VLANID']])->fetch();
			echo '<tr>';
			echo '<td>' . substr($VLAN_Info['VLANID'], 0, 7) . '...</td>';
			echo '<td>' . htmlentities($VLAN_Info['Name']) . '</td>';
			echo '<td>' . htmlentities($VLAN_Info['Description']) . '</td>';
			echo '<td class="text-center"><img src="/icons/manage.png" height="25" onclick="location.href=\'edit.php?id=' . $VLAN_Info['VLANID'] . '\';" /></td>';
			echo '<td class="text-center"><img src="/icons/delete.png" height="25" onclick="if (confirm(\'Delete VLAN?\')) location.href=\'edit.php?id=' . $VLAN_Info['VLANID'] . '&action=delete\';" /></td>';
			echo '</tr>';
		}
		echo '</table>';
	}
	else
	{
		echo '<h1 class="error">You have no VLANs to manage.</h1>';
	}
	echo '<button class="btn btn-lg btn-primary btn-block" onclick="location.href=\'edit.php\';">Add VLAN</button>';
}
?>
<?php
include("../Footer.php");