// interlacer_guiDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "gui.h"
#include "guiDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cinterlacer_guiDlg �Ի���



Cinterlacer_guiDlg::Cinterlacer_guiDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cinterlacer_guiDlg::IDD, pParent)
	, m_input1(_T(""))
	, m_input2(_T(""))
	, m_output(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cinterlacer_guiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_input1);
	DDX_Text(pDX, IDC_EDIT2, m_input2);
	DDX_Text(pDX, IDC_EDIT3, m_output);
}

BEGIN_MESSAGE_MAP(Cinterlacer_guiDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()


// Cinterlacer_guiDlg ��Ϣ�������

BOOL Cinterlacer_guiDlg::OnInitDialog()
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

void Cinterlacer_guiDlg::OnPaint() 
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
HCURSOR Cinterlacer_guiDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Cinterlacer_guiDlg::OnBnClickedButton2()
{	
    TCHAR strFileName[MAX_PATH] = TEXT("");
    TCHAR strPath[MAX_PATH] = TEXT("");

    OPENFILENAME ofn = { sizeof(OPENFILENAME), this->GetSafeHwnd() , NULL,
                         TEXT(".ssif/.m2ts/.264/.h264\0*.ssif;*.m2ts;*.264;*.h264\0All Files\0*.*\0\0"), NULL,
                         0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
                         TEXT("Open Input 1"),
                         OFN_FILEMUSTEXIST|OFN_HIDEREADONLY, 0, 0,
                         TEXT(".wav"), 0, NULL, NULL };

	if( TRUE == GetOpenFileName( &ofn ) )
    {
		m_input1 = strFileName;

		
		TCHAR out[MAX_PATH];
		_tcscpy(out, strFileName);
		
		for(int i=_tcslen(out); i>0; i--)
			if (out[i] == _T('.'))
				out[i] = NULL;

		_tcscat(out, _T(".mvc"));
		m_output = out;
    }
	UpdateData(FALSE);
}

void Cinterlacer_guiDlg::OnBnClickedButton3()
{
    TCHAR strFileName[MAX_PATH] = TEXT("");
    TCHAR strPath[MAX_PATH] = TEXT("");

    OPENFILENAME ofn = { sizeof(OPENFILENAME), this->GetSafeHwnd() , NULL,
                         TEXT(".ssif/.m2ts/.264/.h264\0*.ssif;*.m2ts;*.264;*.h264\0All Files\0*.*\0\0"), NULL,
                         0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
                         TEXT("Open Input 1"),
                         OFN_FILEMUSTEXIST|OFN_HIDEREADONLY, 0, 0,
                         TEXT(".wav"), 0, NULL, NULL };

	if( TRUE == GetOpenFileName( &ofn ) )
    {
		m_input2 = strFileName;
    }
	UpdateData(FALSE);
}

void Cinterlacer_guiDlg::OnBnClickedButton4()
{
	UpdateData(TRUE);
    TCHAR strFileName[MAX_PATH];
	_tcscpy(strFileName, m_output);
    TCHAR strPath[MAX_PATH] = TEXT("");

    OPENFILENAME ofn = { sizeof(OPENFILENAME), this->GetSafeHwnd() , NULL,
                         TEXT(".mvc\0*.mvc\0\0"), NULL,
                         0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
                         TEXT("Open Input 1"),
                         OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, 0, 0,
                         TEXT(".wav"), 0, NULL, NULL };

	if( TRUE == GetSaveFileName( &ofn ) )
    {
		m_output = strFileName;
    }
	UpdateData(FALSE);
}

void Cinterlacer_guiDlg::OnBnClickedButton1()
{
	TCHAR interlacer_path[MAX_PATH+20] = _T("interlacer.exe");
	GetModuleFileName(NULL,interlacer_path,MAX_PATH);
	for(int i=_tcslen(interlacer_path); i>0; i--)
		if (interlacer_path[i] == _T('\\'))
		{
			interlacer_path[i] = NULL;
			break;
		}
	_tcscat(interlacer_path, _T("\\interlacer.exe"));


	FILE *f = _tfopen(interlacer_path, _T("rb"));
	if (f == NULL)
	{
		MessageBox(_T("interlacer.exe not found"), _T("Error"));
		return;
	}
	fclose(f);

	UpdateData(TRUE);

	CString parameter;
	parameter.Format(_T("\"%s\" \"%s\""), m_output, m_input1);
	if(m_input2 != _T(""))
		parameter += _T(" \"") + m_input2 + _T("\"");

	HINSTANCE inst = ShellExecute(NULL, NULL, interlacer_path, parameter, NULL, SW_SHOW);
	if ((int)inst < 32)
		MessageBox(_T("Error executing interlacer.exe ") + parameter, _T("Error"));
	else
		ExitProcess(0);
}
