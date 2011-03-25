#include <winsock.h>
#include <wininet.h>
#include <sys/stat.h>
#include <iostream>
using namespace std;
#define SERVER_PORT 80
#define MAX_CONNECTION 100           //ͬʱ�ȴ������Ӹ���

int make_server_socket() 
{
	struct sockaddr_in server_addr;					//��������ַ�ṹ��
	int tempSockId;									//��ʱ�洢socket������
	tempSockId = socket(PF_INET, SOCK_STREAM, 0);
 
	if (tempSockId == -1)							//�������ֵΪ��1 �����
		return -1;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(&(server_addr.sin_zero), '\0', 8);
	if (bind(tempSockId, (struct sockaddr *)&server_addr,
		sizeof(server_addr)) == -1)
	{
		//�󶨷�����������򷵻أ�1
		return -1;
	}
	if (listen(tempSockId, MAX_CONNECTION) == -1 ) 
	{
		return -1;
	}
	return tempSockId;
 }

/*
void main(int argc, char * argv[]) 
{
	int server_socket = make_server_socket();
	while(true)
	{
		acc_socket = accept(server_socket, (struct sockaddr *)&user_socket, &sock_size); //��������

		MSG_WAITALL
	}

}
*/