<?php

include "db_and_basic_func.php";

if (!strpos($RSA_decoded, "R"))
{
	$paras = explode(",", $RSA_decoded);
	$passkey = $paras[0];
	//$hash = $paras[1];
	$return_key = $paras[2];
}
else
{
	die("E_FAIL:".$result);
}

// check passkey and return
$db = mysql_connect("localhost", "root", "tester88");
mysql_select_db("mydb", $db);

$sql = sprintf("INSERT INTO logs (ip, date, passkey, hash, operation) values ('%s', '%s', '%s', '%s', '%s');", $ip, $date, $passkey, "", "CHECK_REGISTER");
$result = mysql_query($sql);

$user = "unknown";
$rtn = "S_FALSE";
$result = mysql_query("SELECT * FROM active_passkeys where passkey = '".$passkey."'");
$newkey = false;
if (mysql_num_rows($result) <= 0)
{
	$result = mysql_query("SELECT * FROM dropped_passkeys where passkey = '".$passkey."'");
	if (mysql_num_rows($result) > 0)
	{
		$row = mysql_fetch_array($result);
		$user = "(del)".$row["user"];
		
		$rtn = "E_FAIL";
	}
}
else
{
	$row = mysql_fetch_array($result);
	$user = $row["user"];
	$rtn = "S_OK\0";
	
	// passkey update
	$newkey = $com->genkeys($passkey, time() - (12*3600), time() + 24*7*3600);
	$newkey = $com->AES($newkey, $return_key);
}

db_log("PlayerStartup", $rtn, $user, $passkey);
echo $rtn;
if ($newkey)echo $newkey;
?>