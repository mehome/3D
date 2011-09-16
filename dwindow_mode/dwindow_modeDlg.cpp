// dwindow_modeDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "dwindow_mode.h"
#include "dwindow_modeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cdwindow_modeDlg �Ի���



const WCHAR* soft_key= L"Software\\DWindow";
bool save_setting(const WCHAR *key, const void *data, int len, DWORD REG_TYPE)
{
	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, soft_key, 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return false;
	ret = RegSetValueExW(hkey, key, 0, REG_TYPE, (const byte*)data, REG_TYPE!=REG_SZ?len:wcslen((wchar_t*)data)*2+2);
	if (ret != ERROR_SUCCESS)
		return false;

	RegCloseKey(hkey);
	return true;
}

int load_setting(const WCHAR *key, void *data, int len)
{
	HKEY hkey = NULL;
	int ret = RegOpenKeyExW(HKEY_CURRENT_USER, soft_key,0,STANDARD_RIGHTS_REQUIRED |KEY_READ  , &hkey);
	if (ret != ERROR_SUCCESS || hkey == NULL)
		return false;
	ret = RegQueryValueExW(hkey, key, 0, NULL, (LPBYTE)data, (LPDWORD)&len);
	if (ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA)
		return len;

	RegCloseKey(hkey);
	return 0;
}

wchar_t server[200] = L"";
wchar_t port[200] = L"";
wchar_t bar_server[200] = L"";



Cdwindow_modeDlg::Cdwindow_modeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cdwindow_modeDlg::IDD, pParent)
	, m_check_normal(false)
	, m_check_bar(false)
	, m_server(_T(""))
	, m_port(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

void Cdwindow_modeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_server);
	DDX_Text(pDX, IDC_EDIT2, m_port);
	DDX_Control(pDX, IDC_EDIT1, m_server_edit);
	DDX_Control(pDX, IDC_EDIT2, m_port_edit);
}

BEGIN_MESSAGE_MAP(Cdwindow_modeDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_RADIO1, &Cdwindow_modeDlg::OnBnClickedRadio1)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_RADIO2, &Cdwindow_modeDlg::OnBnClickedRadio2)
	ON_BN_CLICKED(IDOK, &Cdwindow_modeDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// Cdwindow_modeDlg ��Ϣ�������

BOOL Cdwindow_modeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void Cdwindow_modeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR Cdwindow_modeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void Cdwindow_modeDlg::OnBnClickedRadio1()
{
	m_port_edit.EnableWindow(FALSE);
	m_server_edit.EnableWindow(FALSE);
}

void Cdwindow_modeDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);


	memset(server, 0, sizeof(server));
	memset(port, 0, sizeof(port));
	memset(bar_server, 0, sizeof(bar_server));
	load_setting(L"BarServer", bar_server, sizeof(bar_server));

	if (bar_server[0])
	{
		wcscpy(server, bar_server);
		wcscpy(port, L"80");
		for(int i=0; i<wcslen(server); i++)
		{
			if (server[i] == L':')
			{
				server[i] = NULL;
				wcscpy(port, server+i+1);
			}
		}
	}
	else
	{
		load_setting(L"Port", port, sizeof(port));
		load_setting(L"Server", server, sizeof(server));
	}

	if (!bar_server[0])
	{
		::SendMessage(GetDlgItem(IDC_RADIO1)->m_hWnd, BM_SETCHECK, TRUE, TRUE);
		m_port_edit.EnableWindow(FALSE);
		m_server_edit.EnableWindow(FALSE);
	}
	else
	{
		::SendMessage(GetDlgItem(IDC_RADIO2)->m_hWnd, BM_SETCHECK, TRUE, TRUE);
		m_port_edit.EnableWindow(TRUE);
		m_server_edit.EnableWindow(TRUE);
	}

	USES_CONVERSION;
	m_server = W2T(server);
	m_port = atoi(W2A(port));
	if (m_port == 0)
		m_port = 80;

	UpdateData(FALSE);
}

void Cdwindow_modeDlg::OnBnClickedRadio2()
{
	m_port_edit.EnableWindow(TRUE);
	m_server_edit.EnableWindow(TRUE);
}

void Cdwindow_modeDlg::OnBnClickedOk()
{
	UpdateData(TRUE);

	if (BST_CHECKED == ::SendMessage(GetDlgItem(IDC_RADIO1)->m_hWnd, BM_GETCHECK, 0, 0))
	{
		// normal mode;
		bar_server[0] = NULL;
	}

	else
	{
		// bar mode;
		wsprintfW(bar_server, L"%s:%d", m_server, m_port);
	}

	wcscpy(server, T2W(m_server.GetBuffer()));
	wsprintfW(port, L"%d", m_port);

	save_setting(L"BarServer", bar_server, sizeof(bar_server), REG_SZ);
	save_setting(L"Server", server, sizeof(server), REG_SZ);
	save_setting(L"Port", port, sizeof(port), REG_SZ);

	OnOK();
}
