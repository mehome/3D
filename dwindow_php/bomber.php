<?php
$form = false;

include "db_and_basic_func.php";

while (list($name, $value) = each($_POST))
{
	if ($name == "username")
		$username = $value;
	if ($name == "password")
		$password = $value;
	if ($name == "admin")
		$admin = $value;
	if ($name == "op")
		$op = $value;
		
		$form = true;
}


if ($form)
{
	// check parameter
	if ($username == "")
		die("�û���Ϊ��");
		
	$username = str_replace("'", "''", $username);
	
	// test user
	$userexist = false;
	$result = mysql_query("SELECT * FROM users where name='".$username."'");
	if (mysql_num_rows($result) > 0)
		$userexist = true;
	
	// adding
	if ($op == "add")
	{
		if ($password == "")
			die("����Ϊ��");

		if ($userexist)
		{
			printf("�û� '%s' �Ѵ���.", $username);
			goto theend;
		}
		
		
		$pattern = "0123456789ABCDEF";
		$salt = "";
		for($i=0; $i < 64; $i++)
			$salt .= $pattern{mt_rand(0,15)};
		$result = mysql_query(sprintf("INSERT INTO users (name,pass_hash, usertype, bar_max_users, salt, deleted, expire) values ('%s', '%s'".
		", 0, 0, '%s', 0, %d)",$username, $com->SHA1($com->SHA1($password) . $salt), $salt, time()));
		if ($result)
		{
			printf("ע���û� %s �ɹ�!", $username, $password);
			db_log("UserAdd", "OK", $username, $com->SHA1($password));
		}
		else
		{
			printf("adding user %s failed.", $username);
		}
	}
	
	// deleting
	if ($op == "del")
	{
		$string = mysql_query("INSERT INTO dropped_passkeys SELECT * FROM active_passkeys WHERE user='".$username."'");
		if (!$result)
		{
			printf("delete user %s failed.");
			goto theend;
		}

		$result = mysql_query("DELETE FROM active_passkeys where user='".$username."'");
		if (!$result)
		{
			printf("delete user %s failed.");
			goto theend;
		}
		$result = mysql_query("DELETE FROM users where name='".$username."'");
		if (!$result)
		{
			printf("delete user %s failed.");
			goto theend;
		}
		
		db_log("UserDel", $userexist ? "OK" : "NO USER", $username);
		printf("delete user %s, OK!", $username, $password);
	}
	
	// die
	theend:
	die("<BR>");
}

db_log("WWW", "OK", 0, "bomber.php");
?>
<html>
	<script language=javascript>
	function check(form)
	{
		if (form.username.value == "")
		{
			alert("�û���Ϊ��");
		}
		else if(form.password.value == form.password2.value)
		{
			form.submit();
		}		
		else
		{
			alert("���벻��ȷ");
		}
	}
	</script>
	
	<form method="POST" name=form1>
		�û���       <input type="text" name="username" /> <br />
		����        <input type="password" name="password" /> <br />
		�ظ�����        <input type="password" name="password2" /> <br />
		<input type="hidden" name = "op" value="add" />
		<input type="button" value="ע��" onclick="check(this.form)"/>
	</form>
</html>