
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

/*
              ������0        ������123
ȥ���2           x            o
ӰԺ/����13       x            x
�Ѽ���0+          x            x  
δ����0           O            O
*/

$type2str = array("ȥ���", "7��", "1��", "1��");
$usertype2str = array("����", "ӰԺ", "����", "����", "�ѵ��ڵĸ����û�");
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
		die("��Ч�ļ����������".$return_button);
	$row = mysql_fetch_array($result);
	if ($row["used"] != 0)
		die("�˼������ѱ�ʹ��".$return_button);
	$cardtype = $row["type"];
		
			
		
	if ($usertype == 2 && $cardtype == 0)
	{
		printf("���û��Ѽ��������ʹ�þ���������%s",$return_button);
		die();
	}
	else if ($usertype == 2 && cardtype != 0)
	{
		//printf("���û��Ѽ����ʱ�޷�ʹ�ø��˼����롣%s",$return_button);
		//die();
	}
	else if ($usertype !=0 && $cardtype != 0)
	{
		printf("���˼�����ֻ�����ڸ����û���� �û�%sΪ%s�û�%s",
				$username,$usertype2str[$usertype],$return_button);
		die();
	}
	else if ($usertype !=0 && $cardtype == 0)
	{
		printf("����������ֻ������δ�����˻��� �����û���ʹ��ר�õ�ʱ�伤���룬 �û�%sΪ%s�û�%s",
				$username,$usertype2str[$usertype],$return_button);
		die();
	}		
	else if ($usertype == 0 && $expire > time() && $cardtype == 0)
	{
		printf("�û��Ѽ�� �û�%sΪ%s�û�%s",
					$username,$usertype2str[$usertype],$return_button);
		die();
	}
	else if ($usertype == 0 && $expire > time() && $cardtype != 0)
	{
		printf("�û��Ѽ�� �û�%sΪ%s�û�%s",
					$username,$usertype2str[$usertype],$return_button);
		die();
	}
	// do it
	if($_POST["op"] == "valid")
	{
		// TODO
		$type2time = array(0xffffff, 7*3600*24, 31*3600*24, 366*3600*24);
		$extra_time = $type2time[$cardtype];
		$new_expire = 1931316605;//max($expire, time()) + $extra_time;
		$op_ok = true;
		
		if (/*$usertype == 0 && $expire < time()  &&*/ $cardtype == 0)
		{
			// δ�����û�����漤����
			$sql = sprintf("update users set usertype=2 where name='%s'", $username);
			$result = mysql_query($sql);
			//echo "1".mysql_error()."<BR>";
			$sql = sprintf("update users set expire=%d where name='%s'", $new_expire, $username);
			$result = mysql_query($sql);
			//echo "2".mysql_error()."<BR>";
			$sql = sprintf("update cards set used = 1 where code = '%s'", $card);
			$result = mysql_query($sql);
			//echo "3".mysql_error()."<BR>";
			db_log("USER_CHANGE", "OK", 0, $username, "����", $card);
			
			/*
			$sql = sprintf("update users set usertype=2 where name='%s';", $username)
						.sprintf("update users set expire=%d where name='%s';", $new_expire, $username)
						.sprintf("update cards set used = 1 where code = '%s'", $card);
			$result = mysql_query($sql);
			echo "3".mysql_error()."<BR>";
			*/
		}
		else if (/*$usertype == 0 &&*/ $cardtype == 1)
		{
			// ���˼�����
			$sql = sprintf("update users set expire=%d where name='%s'", $new_expire, $username);
			$result = mysql_query($sql);
			//echo "4".mysql_error()."<BR>";
			$sql = sprintf("update cards set used = 1 where code = '%s'", $card);
			$result = mysql_query($sql);
			//echo "5".mysql_error()."<BR>";
			$sql = sprintf("update users set usertype=0 where name='%s'", $username);
			$result = mysql_query($sql);
			//echo "6".mysql_error()."<BR>";
			db_log("USER_CHANGE", "OK", 0, $username, "����", $card);
		}
		else
		{
			db_log("USER_CHANGE", "FAIL", 0, $username, $card);
			die( "����ʧ��");
		}
		
		
		die("�������");
	}

?>

<html>
	<form method="POST" name=form1>
		<input type="hidden" name="op" value="post" />
		
		<?php

		// card type and time
		if ($cardtype == 0)
			echo("�˼�����Ϊ��δʹ�õľ���������<br>");
		else
			printf("�˼�����Ϊ��δʹ�õĸ��˼����� %s<br>", "", $type2str[$cardtype]);

		printf("������<br>");
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
		������         <input type="text" name="card" /> <br />
		����         <input type="text" name="password" /> <br />
		
		<input type="submit" value="��ֵ��" onclick="this.form.submit()"/>
	</form>
</html>