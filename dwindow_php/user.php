<?php

include "db_and_basic_func.php";

$form = false;

while (list($name, $value) = each($_POST))
{
	if ($name == "username")
		$username = $value;
	if ($name == "password")
		$password = $value;
	if ($name == "newpassword")
		$newpassword = $value;
	if ($name == "repeat")
		$repeat = $value;

		$form = true;
}

if ($form)
{
	// check parameter
	if (strstr($username, "'") or strstr($newpassword, "'") or strstr($password, "'"))
	{
		$username = str_replace("'", "''", $username);
		$password = str_replace("'", "''", $password);
		$newpassword = str_replace("'", "''", $newpassword);
		
		db_log("HACK:user.php", "BLOCKED", $username, $password, $newpassword);
		die("''''''''''");
	}
	if ($username == "")
		die("�û�����Ϊ��");
	if ($newpassword != $repeat)
		die("�������벻ͬ");

	$oldsha1 = $com->SHA1($password);
	$newsha1 = $com->SHA1($newpassword);

	$result = mysql_query("SELECT * FROM users where name = '".$username."'");		//warning: possible SQL injection
	if (mysql_num_rows($result) <= 0)
	{
			db_log("PasswordCrack", "User not found", $username, $password, $newpassword);
			die("�û�/ �������");
	}
	$row = mysql_fetch_array($result);
	$salt = $row["salt"];
	if ($com->SHA1($oldsha1.$salt) != $row["pass_hash"])
	{
			db_log("PasswordCrack", "BLOCKED", $username, $password, $newpassword);
			die("�û�/ �������");
	}


	// deleting active passkey
	$result = mysql_query("DELETE FROM active_passkeys where user='".$username."'");
	if (!$result)
		goto theend;

	// update password
	$sql = sprintf("UPDATE users set pass_hash='%s' where name='%s'", $com->SHA1($newsha1.$salt), $username);
	$result = mysql_query($sql);
	if (!$result)
		goto theend;

	db_log("PasswordChange", "OK", $username, $oldsha1, $newsha1);
	printf("�ɹ�, %s���������޸ģ������¼��", $username);

	// die
	theend:
	die("<BR>");
}
db_log("WWW", "OK", 0, "user.php");
?>

<html>
	<form method="POST" name=form1>
		�û���       <input type="text" name="username" /> <br />
		����         <input type="password" name="password" /> <br />
		������       <input type="password" name="newpassword" /> <br />
		ȷ������     <input type="password" name="repeat" /> <br />
		<input type="button" value="�޸ģ�" onclick="this.form.submit()"/>
	</form>
</html>