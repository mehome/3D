<?php
include "checkadmin.php";

if (!isset($_GET["id"]))
	die("no id");
	
$id = $_GET["id"];
$result = mysql_query(sprintf("SELECT * FROM users where id = %s", $id));
if (mysql_num_rows($result) <= 0)
	die("user not found");

$printinfo = true;
	
if (isset($_POST["op"]))
{
	$result = mysql_query(sprintf("SELECT * FROM users where id = %s", $id));
	
	if ($_POST["op"] == "ban")
	{
		
		$result = mysql_query(sprintf("update users set deleted = 1-deleted where id = %s", $id));
		$result = mysql_query(sprintf("SELECT * FROM users where id = %s", $id));
	}
	else if ($_POST["op"] == "del")
	{
		$result = mysql_query(sprintf("delete from users where id = %s", $id));
		die("user deleted, <a href=\"admin.php\"> click here to return.</a>");
		$printinfo = false;
	}
	else if ($_POST["op"] == "update")
	{
		$newdate = $_POST["newdate"];
		list($y, $m, $d) = sscanf($newdate, "%d-%d-%d");
		$newdate = mktime(0,0,0,$m,$d,$y);
		
		$result = mysql_query(sprintf("update users set expire = %s where id = %s", $newdate, $id));
		$result = mysql_query(sprintf("SELECT * FROM users where id = %s", $id));
	}
}


if ($printinfo)
{
	$row = mysql_fetch_array($result);
	printf("%s<BR>\n", $row["name"]);
	
	$usertype = $row["usertype"];
	printf("�û����ͣ�%d(", $usertype);
	if ($usertype == 0)
		printf("��ͨ�û�");
	else if ($usertype == 1)
		printf("ӰԺ�û�");
	else if ($usertype == 2)
		printf("ȥ����û�");
	else if ($usertype == 3)
	{
		printf("�����û�");
		printf(", %d�û����", $row["bar_max_users"]);
	}
	else
		printf("<font color=red>�Ƿ��û�</font>");
			
	printf(")<br>\n");
			
	$date = Date("Y-m-d", $row["expire"]);
	

}

?>
<html>
	<script language=javascript>
	function warn_del(form)
	{
		if(confirm("are you sure want to delete this user? this operation is not revertable.") == 1)
		{
			form.op.value='del';
			form.submit();
		}
	}
	
	
	</script>
	<form method="POST" name=form1>
		<input type="hidden" name = "op" value="add" />
		�������ڣ�<input type="text" value="<?php echo $date?>" name="newdate"/>
		<input type="button" value="����", onclick="this.form.op.value='update';this.form.submit()"/><p>
<?php
	if ($row["deleted"] != 0)
		printf("<font color=red> (�ѽ���)</font><p>\n");
?>
		<input type="button" value="����/����û�" onclick="this.form.op.value='ban';this.form.submit()"/>
		<input type="button" value="ɾ���û�" onclick="warn_del(this.form)"/>
		<p>
		<p>
		<a href = "list_user.php"> �����û��б�</a>
	</form>
</html>