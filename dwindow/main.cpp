#include <time.h>
#include "global_funcs.h"
#include "dx_player.h"
#include "..\AESFile\E3DReader.h"
#include "..\AESFile\rijndael.h"
#include "resource.h"

#include <wininet.h>
#pragma comment(lib,"wininet.lib")

char url[300] = "http://59.51.45.21:80/w32.php?";

void download(char *url_to_download, char *out)
{
	HINTERNET HI;
	HI=InternetOpenA("dwindow",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
	if (HI==NULL)
		return;

	HINTERNET HURL;
	HURL=InternetOpenUrlA(HI, url_to_download,NULL,0,INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_AUTO_REDIRECT,0);
	if (HURL==NULL)
		return;


	DWORD byteread = 0;
	BOOL internetreadfile = InternetReadFile(HURL,out, 64,&byteread);
}

// main window
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
int on_command(HWND hWnd, int uid);
int init_dialog(HWND hWnd);

UINT FindAwardBios( BYTE** ppBiosAddr )
{
	BYTE* pBiosAddr = * ppBiosAddr + 0xEC71;

	BYTE szBiosData[128];
	CopyMemory( szBiosData, pBiosAddr, 127 );
	szBiosData[127] = 0;

	int iLen = strlen( ( char* )szBiosData );
	if( iLen > 0 && iLen < 128 )
	{
		//AWard:         07/08/2002-i845G-ITE8712-JF69VD0CC-00 
		//Phoenix-Award: 03/12/2002-sis645-p4s333
		if( szBiosData[2] == '/' && szBiosData[5] == '/' )
		{
			BYTE* p = szBiosData;
			while( * p )
			{
				if( * p < ' ' || * p >= 127 )
				{
					break;
				}
				++ p;
			}
			if( * p == 0 )
			{
				* ppBiosAddr = pBiosAddr;
				return ( UINT )iLen;
			}
		}
	}
	return 0;
}

UINT FindAmiBios( BYTE** ppBiosAddr )
{
	BYTE* pBiosAddr = * ppBiosAddr + 0xF478;

	BYTE szBiosData[128];
	CopyMemory( szBiosData, pBiosAddr, 127 );
	szBiosData[127] = 0;

	int iLen = strlen( ( char* )szBiosData );
	if( iLen > 0 && iLen < 128 )
	{
		// Example: "AMI: 51-2300-000000-00101111-030199-"
		if( szBiosData[2] == '-' && szBiosData[7] == '-' )
		{
			BYTE* p = szBiosData;
			while( * p )
			{
				if( * p < ' ' || * p >= 127 )
				{
					break;
				}
				++ p;
			}
			if( * p == 0 )
			{
				* ppBiosAddr = pBiosAddr;
				return ( UINT )iLen;
			}
		}
	}
	return 0;
}

UINT FindPhoenixBios( BYTE** ppBiosAddr )
{
	UINT uOffset[3] = { 0x6577, 0x7196, 0x7550 };
	for( UINT i = 0; i < 3; ++ i )
	{
		BYTE* pBiosAddr = * ppBiosAddr + uOffset[i];

		BYTE szBiosData[128];
		CopyMemory( szBiosData, pBiosAddr, 127 );
		szBiosData[127] = 0;

		int iLen = strlen( ( char* )szBiosData );
		if( iLen > 0 && iLen < 128 )
		{
			// Example: Phoenix "NITELT0.86B.0044.P11.9910111055"
			if( szBiosData[7] == '.' && szBiosData[11] == '.' )
			{
				BYTE* p = szBiosData;
				while( * p )
				{
					if( * p < ' ' || * p >= 127 )
					{
						break;
					}
					++ p;
				}
				if( * p == 0 )
				{
					* ppBiosAddr = pBiosAddr;
					return ( UINT )iLen;
				}
			}
		}
	}
	return 0;
}

INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		on_command(hDlg, LOWORD(wParam));
		break;

	case WM_INITDIALOG:
		init_dialog(hDlg);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	CoInitialize(NULL);

	RECT screen1;
	RECT screen2;
	if (FAILED(get_monitors_rect(&screen1, &screen2)))
	{
		MessageBoxW(0, L"System initialization failed, the program will exit now.", L"Error", MB_OK);
		return -1;
	}

	load_passkey();
	if (FAILED(check_passkey()))
	{
		int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_DIALOG2), NULL, MainDlgProc );
		return 0;
	}

	g_bomb_function;
	save_passkey();


	dx_player test(screen1, screen2, hinstance);
	while (!test.is_closed())
		Sleep(100);

	return 0;
}

int main()
{
	/*
	file_reader reader;
	//reader.SetFile(CreateFileW (L"Z:\\00013.e3d", GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
	reader.SetFile(CreateFileW (L"F:\\Movie\\���ɵ���.Despicable_me.3DBDRip.SBS.720P-3D4Dnet.test.e3d", GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));

	load_passkey();
	dwindow_message_uncrypt message;
	message.zero = 0;
	memcpy(message.passkey, g_passkey, 32);
	memcpy(message.requested_hash, reader.m_hash, 20);
	srand(time(NULL));
	for(int i=0; i<32; i++)
		message.random_AES_key[i] = rand() & 0xff;

	char tmp[300];
	memcpy(tmp, &message, 128);

	unsigned char encrypted_message[128];
	RSA_dwindow_public(tmp, encrypted_message);

	for(int i=0; i<128; i++)
	{
		sprintf(tmp, "%02X", encrypted_message[i]);
		strcat(url, tmp);
	}

	char str_e3d_key[65] = "D3821F7B81206903280461E52DE2B29901B9B458836B3795DD40F50C2583EF7A";
	download(url, str_e3d_key);
	unsigned char e3d_key[36];
	for(int i=0; i<32; i++)
		sscanf(str_e3d_key+i*2, "%02X", e3d_key+i);

	AESCryptor aes;
	aes.set_key(message.random_AES_key, 256);
	aes.decrypt(e3d_key, e3d_key);
	aes.decrypt(e3d_key+16, e3d_key+16);
	reader.set_key(e3d_key);

	e3d_set_process_key(e3d_key);
	*/

	/*
	BYTE szSystemInfo[4096]; // �ڳ���ִ����Ϻ󣬴˴��洢ȡ�õ�ϵͳ������
	UINT uSystemInfoLen = 0; // �ڳ���ִ����Ϻ󣬴˴��洢ȡ�õ�ϵͳ������ĳ���	//�ṹ���� 
	typedef struct _UNICODE_STRING 
	{ 
		USHORT  Length;//���� 
		USHORT  MaximumLength;//��󳤶� 
		PWSTR  Buffer;//����ָ�� 
	} UNICODE_STRING,*PUNICODE_STRING; 
	typedef struct _OBJECT_ATTRIBUTES 
	{ 
		ULONG Length;//���� 18h 
		HANDLE RootDirectory;//  00000000 
		PUNICODE_STRING ObjectName;//ָ���������ָ�� 
		ULONG Attributes;//��������00000040h 
		PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR��0 
		PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE��0 
	} OBJECT_ATTRIBUTES; 
	typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES; 
	//����ָ���������
	typedef DWORD  (__stdcall *ZWOS )( PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES); 
	typedef DWORD  (__stdcall *ZWMV )( HANDLE,HANDLE,PVOID,ULONG,ULONG,PLARGE_INTEGER,PSIZE_T,DWORD,ULONG,ULONG); 
	typedef DWORD  (__stdcall *ZWUMV )( HANDLE,PVOID); 

	// BIOS ��ţ�֧�� AMI, AWARD, PHOENIX
	{
		SIZE_T ssize; 

		LARGE_INTEGER so; 
		so.LowPart=0x000f0000;
		so.HighPart=0x00000000; 
		ssize=0xffff; 
		wchar_t strPH[30]=L"\\device\\physicalmemory"; 

		DWORD ba=0;

		UNICODE_STRING struniph; 
		struniph.Buffer=strPH; 
		struniph.Length=0x2c; 
		struniph.MaximumLength =0x2e; 

		OBJECT_ATTRIBUTES obj_ar; 
		obj_ar.Attributes =64;
		obj_ar.Length =24;
		obj_ar.ObjectName=&struniph;
		obj_ar.RootDirectory=0; 
		obj_ar.SecurityDescriptor=0; 
		obj_ar.SecurityQualityOfService =0; 

		HMODULE hinstLib = LoadLibraryA("ntdll.dll"); 
		ZWOS ZWopenS=(ZWOS)GetProcAddress(hinstLib,"ZwOpenSection"); 
		ZWMV ZWmapV=(ZWMV)GetProcAddress(hinstLib,"ZwMapViewOfSection"); 
		ZWUMV ZWunmapV=(ZWUMV)GetProcAddress(hinstLib,"ZwUnmapViewOfSection"); 

		//���ú������������ڴ����ӳ�� 
		HANDLE hSection; 
		if( 0 == ZWopenS(&hSection,4,&obj_ar) && 
			0 == ZWmapV( 
			( HANDLE )hSection,   //��Sectionʱ�õ��ľ�� 
			( HANDLE )0xFFFFFFFF, //��Ҫӳ����̵ľ���� 
			&ba,                  //ӳ��Ļ�ַ 
			0,
			0xFFFF,               //����Ĵ�С 
			&so,                  //�����ڴ�ĵ�ַ 
			&ssize,               //ָ���ȡ�ڴ���С��ָ�� 
			1,                    //�ӽ��̵Ŀɼ̳����趨 
			0,                    //�������� 
			2                     //�������� 
			) )
			//ִ�к���ڵ�ǰ���̵Ŀռ俪��һ��64k�Ŀռ䣬����f000:0000��f000:ffff��������ӳ�䵽���� 
			//ӳ��Ļ�ַ��ba����,���ӳ�䲻������,Ӧ����ZwUnmapViewOfSection�Ͽ�ӳ�� 
		{
			BYTE* pBiosSerial = ( BYTE* )ba;
			UINT uBiosSerialLen = FindAwardBios( &pBiosSerial );
			if( uBiosSerialLen == 0U )
			{
				uBiosSerialLen = FindAmiBios( &pBiosSerial );
				if( uBiosSerialLen == 0U )
				{
					uBiosSerialLen = FindPhoenixBios( &pBiosSerial );
				}
			}
			if( uBiosSerialLen != 0U )
			{
				CopyMemory( szSystemInfo + uSystemInfoLen, pBiosSerial, uBiosSerialLen );
				uSystemInfoLen += uBiosSerialLen;
			}
			ZWunmapV( ( HANDLE )0xFFFFFFFF, ( void* )ba );
		}
	}

	// CPU ID
	{
		BOOL bException = FALSE;
		BYTE szCpu[16]  = { 0 };
		UINT uCpuID     = 0U;

		__try 
		{
			_asm 
			{
				mov eax, 0
					cpuid
					mov dword ptr szCpu[0], ebx
					mov dword ptr szCpu[4], edx
					mov dword ptr szCpu[8], ecx
					mov eax, 1
					cpuid
					mov uCpuID, edx
			}
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			bException = TRUE;
		}

		if( !bException )
		{
			CopyMemory( szSystemInfo + uSystemInfoLen, &uCpuID, sizeof( UINT ) );
			uSystemInfoLen += sizeof( UINT );

			uCpuID = strlen( ( char* )szCpu );
			CopyMemory( szSystemInfo + uSystemInfoLen, szCpu, uCpuID );
			uSystemInfoLen += uCpuID;
		}
	}


	SYSTEM_INFO sinfo;
	GetSystemInfo(&sinfo);

	printf("len=%d", uSystemInfoLen);
	*/

	WinMain(LoadLibraryW(L"dwindow.exe"), 0, "", SW_SHOW);
}


int init_dialog(HWND hWnd)
{
	SetWindowTextW(hWnd, C(L"Enter User ID"));

	return 0;
}

int on_command(HWND hWnd, int uid)
{
	if (uid == IDOK)
	{
		wchar_t str_key[512];
		GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT1), str_key, 512);
 
		char new_key[256];
		for(int i=0; i<128; i++)
			swscanf(str_key+i*2, L"%02X", new_key+i);
		memcpy(g_passkey_big, new_key, 128);
		save_passkey();

		MessageBoxW(hWnd, L"This program will exit now, Restart it to use new user id.", L"Exiting", MB_ICONINFORMATION);

		EndDialog(hWnd, IDOK);
	}
	else if (uid == IDCANCEL)
	{
		EndDialog(hWnd, IDCANCEL);
	}

	return 0;
}