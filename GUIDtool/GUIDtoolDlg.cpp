// GUIDtoolDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "GUIDtool.h"
#include "GUIDtoolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CGUIDtoolDlg �Ի���




CGUIDtoolDlg::CGUIDtoolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGUIDtoolDlg::IDD, pParent)
	, m_GUID(_T(""))
	, m_GUID_define(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGUIDtoolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_GUID);
	DDX_Text(pDX, IDC_EDIT2, m_GUID_define);
}

BEGIN_MESSAGE_MAP(CGUIDtoolDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_EDIT1, &CGUIDtoolDlg::OnEnChangeEdit1)
END_MESSAGE_MAP()


// CGUIDtoolDlg ��Ϣ�������

BOOL CGUIDtoolDlg::OnInitDialog()
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

void CGUIDtoolDlg::OnPaint()
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
HCURSOR CGUIDtoolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CGUIDtoolDlg::OnEnChangeEdit1()
{
	UpdateData(TRUE);

	CLSID guid;
	CString guid_string = m_GUID;
	guid_string.Trim();

	if (SUCCEEDED(CLSIDFromString((LPOLESTR)(const TCHAR*)guid_string,&guid)))
	{
		m_GUID_define.Format(_T("DEFINE_GUID(CLSID_YOURNAME, 0x%08x, 0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x)"),
			guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7], guid.Data4[8]);
	}
	else
	{
		m_GUID_define = _T("Invalid GUID string");
	}
	UpdateData(FALSE);
	
}
