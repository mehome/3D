
<?php
include "db_and_basic_func.php";
// card type:
// 0 = ad
// 1 = week
// 2 = month
// 3 = year

// user type:
// 0 = normal
// 1 = theater
// 2 = no ad
// 3 = bar
// 4 = expired normal
$type2str = array("ȥ���", "7��", "1��", "1��");
$usertype2str = array("����", "ӰԺ", "ȥ���", "����", "�ѵ��ڵĸ����û�");
$return_button="<BR><input type='button' value='����' onclick='history.go(-1)'/>";

if (isset($_POST["op"]))
{
	// check user and card first
	$username = str_replace("'", "''", $_POST["username"]);
	$card = str_replace("'", "''", $_POST["card"]);
	$password = str_replace("'", "''", $_POST["password"]);
		
	$result = mysql_query(sprintf("select * from users where name = '%s'", $username));
	if (mysql_num_rows($result) <= 0)
		die("�û� ". $username . " δ�ҵ�".$return_button);
	$row = mysql_fetch_array($result);
	$expire = $row["expire"];
	$usertype = $row["usertype"];

	$result = mysql_query(sprintf("select * from cards where code = '%s' and pass= '%s'",
						$card, $password));
	if (mysql_num_rows($result) <= 0)
		die("��Ч�Ŀ��Ż�����".$return_button);
	$row = mysql_fetch_array($result);
	if ($row["used"] != 0)
		die("�˿��ѱ�ʹ��".$return_button);
	$cardtype = $row["type"];
		
			
		
	if ($usertype == 2 && $cardtype == 0)
	{
		printf("���û��Ѽ���ȥ��棬������ʹ��ȥ��濨%s",$return_button);
		die();
	}
	if ($usertype == 0 && $expire > time() && $cardtype == 0)
	{
		printf("ȥ��濨�޷������Ѹ��Ѹ����û��� �û�%sΪ%s�û�%s",
					$username,$usertype2str[$usertype],$return_button);
		die();
	}
	else if ($usertype !=0 && $usertype != 2 && $cardtype != 0)
	{
		printf("ʱ�俨ֻ������ȥ����û�תΪ�����û��������ڸ����û�����ʱ�䣬 �û�%sΪ%s�û�%s",
				$username,$usertype2str[$usertype],$return_button);
		die();
	}
	else if ($usertype !=0 && $usertype != 2 && $cardtype == 0)
	{
		printf("ȥ��濨ֻ������ȥ��棬 �����û���ʹ��ר�õ�ʱ�俨�� �û�%sΪ%s�û�%s",
				$username,$usertype2str[$usertype],$return_button);
		die();
	}		
	if($_POST["op"] == "valid")
	{
		// TODO
		die();
	}

?>

<html>
	<form method="POST" name=form1>
		<input type="hidden" name="op" value="post" />
		
		<?php

		// card type and time
		if ($cardtype == 0)
			echo("�˿�Ϊ��δʹ�õ�ȥ��濨<br>");
		else
			printf("�˿�Ϊ��δʹ�õ�ʱ�俨��%s��<br>", $type2str[$cardtype]);

		printf("������ֵ��<br>");
		?>
		
		<input type="hidden" name="op" value="valid" />
		<input type="hidden" name="username" value="<?php echo $_POST["username"] ?>"/> <br />
		<input type="hidden" name="card" value="<?php echo $_POST["card"] ?>" /> <br />
		<input type="hidden" name="password" value="<?php echo $_POST["password"] ?>"/> <br />
		<input type="submit" value="����"/>
		<input type="button" value="����" onclick="history.go(-1)"/>
	</form>
</html>

<?php
die();
}
?>

<html>
	<form method="POST" name=form1>
		<input type="hidden" name="op" value="post" />
		�û���       <input type="text" name="username" /> <br />
		����         <input type="text" name="card" /> <br />
		����         <input type="text" name="password" /> <br />
		
		<input type="submit" value="��ֵ��" onclick="this.form.submit()"/>
	</form>
</html>