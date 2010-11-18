// remux_publishDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "remux_publish.h"
#include "remux_publishDlg.h"
#include ".\remux_publishdlg.h"

#include "..\libchecksum\libchecksum.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cremux_publishDlg �Ի���



Cremux_publishDlg::Cremux_publishDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cremux_publishDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cremux_publishDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cremux_publishDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
END_MESSAGE_MAP()


// Cremux_publishDlg ��Ϣ�������

BOOL Cremux_publishDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	
	return TRUE;  // ���������˿ؼ��Ľ��㣬���򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void Cremux_publishDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
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

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
HCURSOR Cremux_publishDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Cremux_publishDlg::OnDropFiles(HDROP hDropInfo)
{


	int cFiles = DragQueryFileW(hDropInfo, (UINT)-1, NULL, 0); 

	{
		DragQueryFileW(hDropInfo, 0, pathname, sizeof(pathname));
		check_file();
	}

	CDialog::OnDropFiles(hDropInfo);
}

void Cremux_publishDlg::check_file()
{
	check_result = ::verify_file(pathname);

	if (check_result == 0)
	{
		SetDlgItemText(IDC_STATIC, _T("State: new file"));
		::EnableWindow(GetDlgItem(IDC_BUTTON2)->m_hWnd, TRUE);
	}
	else if (check_result > 1)
	{
		SetDlgItemText(IDC_STATIC, _T("State: signature OK"));
		::EnableWindow(GetDlgItem(IDC_BUTTON2)->m_hWnd, FALSE);
	}
	else if (check_result == 1)
	{
		SetDlgItemText(IDC_STATIC, _T("State: corrupted file"));
		::EnableWindow(GetDlgItem(IDC_BUTTON2)->m_hWnd, TRUE);
	}
	else if (check_result <0)
	{
		SetDlgItemText(IDC_STATIC, _T("State: invalid file"));
		::EnableWindow(GetDlgItem(IDC_BUTTON2)->m_hWnd, FALSE);
	}

}


void Cremux_publishDlg::publish_signature(const DWORD *checksum, DWORD *signature)
{
	unsigned int dwindow_d[32] = {0x46bbf241, 0xd39c0d91, 0x5c9b9170, 0x43187399, 0x6568c96b, 0xe8a5445b, 0x99791d5d, 0x38e1f280, 0xb0e7bbee, 0x3c5a66a0, 0xe8d38c65, 0x5a16b7bc, 0x53b49e94, 0x11ef976d, 0xd212257e, 0xb374c4f2, 0xc67a478a, 0xe9905e86, 0x52198bc5, 0x1c2b4777, 0x8389d925, 0x33211e75, 0xc2cab10e, 0x4673bf76, 0xfdd2332e, 0x32b10a08, 0x4e64f572, 0x52586369, 0x7a3980e0, 0x7ce9ba99, 0x6eaf6bfe, 0x707b1206};
	DWORD m[32];

	memset(m, 0, 128);
	memcpy(m, checksum, 20);

	RSA(signature, m, (DWORD*)dwindow_d, (DWORD*)dwindow_n, 32);
}
void Cremux_publishDlg::OnBnClickedButton2()
{
	// calculate signature
	char sha1[20];
	DWORD signature[32];
	video_checksum(pathname, (DWORD*)sha1);
	publish_signature((DWORD*) sha1, signature);

	// check RSA error( should not happen)
	bool result = verify_signature((DWORD*)sha1, signature);
	if(!result)
	{
		printf("RSA error...\n");
		return; 
	}

	// write signature
	int pos = find_startcode(pathname);
	FILE *f = _wfopen(pathname, L"r+b");
	fseek(f, pos, SEEK_SET);
	fwrite(signature, 1, 128, f);
	fflush(f);
	fclose(f);

	check_file();
}
