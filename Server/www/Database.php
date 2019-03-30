<?php
error_reporting(E_ALL);
ini_set('default_charset', 'utf-8');
class Database
{
	private $connection;
	function __construct()
	{
		$this->connection = new PDO("mysql:dbname=MegaLAN;host=127.0.0.1;charset=utf8", "MegaLAN", "MegaLAN");
		$this->connection->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	}
	function Query($sql, $args = NULL){
		$stmt = $this->connection->prepare($sql);
		if (!$stmt)
			throw new Exception ("SQL Error");
		if (!$stmt->execute($args))
			throw new Exception ("SQL Error " . $stmt->error);
		return $stmt;
	}
}