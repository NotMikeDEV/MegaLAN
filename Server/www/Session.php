<?php
require("Database.php");
function session_openDB($savePath, $sessionName)
{
	return TRUE;
}
function session_closeDB()
{
	return TRUE;
}
function session_readDB($sessionID)
{
	global $db;
	$row = $db->Query("SELECT Data FROM Sessions WHERE SID = :SID", [':SID'=>$sessionID])->fetch();
	if ($row)
		return $row['Data'];
	return false;
}
function session_writeDB($sessionID, $data)
{
	global $db;
	$row = $db->Query("REPLACE INTO Sessions (SID, Data, Timestamp) VALUES (:SID, :Data, :Time)", [':SID'=>$sessionID, ':Data'=>$data, ':Time'=>time()]);
	return true;
}
function session_destroyDB($sessionID)
{
	global $db;
	$db->Query("DELETE FROM Sessions WHERE SID = :SID", [':SID'=>$sessionID]);
	return true;
}
function session_gcDB($lifetime)
{
	global $db;
	$db->Query("DELETE FROM Sessions WHERE Time < = :Time", [':Time'=>time() - $lifetime]);
	return true;
}
session_set_save_handler ('session_openDB', 'session_closeDB', 'session_readDB', 'session_writeDB', 'session_destroyDB', 'session_gcDB');
session_start();
