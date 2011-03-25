// lrtbDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "resource.h"
#include <math.h>
#include "lrtb.h"
#include "lrtbDlg.h"
#include "LayoutDetect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ClrtbDlg �Ի���




ClrtbDlg::ClrtbDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ClrtbDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void ClrtbDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress);
}

BEGIN_MESSAGE_MAP(ClrtbDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DROPFILES()
END_MESSAGE_MAP()


// ClrtbDlg ��Ϣ�������

BOOL ClrtbDlg::OnInitDialog()
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

void ClrtbDlg::OnPaint()
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
HCURSOR ClrtbDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void ClrtbDlg::OnBnClickedButton1()
{
	//layout_detector detector(100, L"E:\\test_folder\\24.avi");
	//layout_detector detector(100, L"D:\\Users\\my12doom\\Documents\\�ǳ�С����3_���¸�ʽ720-480.ts.mkv");
	//layout_detector detector(100, L"F:\\TDDOWNLOAD\\3840.ts");
	//layout_detector detector(100, L"F:\\TDDOWNLOAD\\00019hsbs.mkv");

}

void ClrtbDlg::OnDropFiles(HDROP hDropInfo)
{
	int cFiles = DragQueryFile(hDropInfo, (UINT)-1, NULL, 0); 

	if ( cFiles > 0)
	{
		SetWindowText(_T("�����"));
		TCHAR FilePath[MAX_PATH];
		DragQueryFile(hDropInfo, 0, FilePath, sizeof(FilePath));
		CString file = FilePath;

		int nscan = 100;
		layout_detector detector(nscan, (LPCWSTR)file);
		while (detector.m_scaned < nscan)
		{
			m_progress.SetRange(1, nscan);
			m_progress.SetPos(detector.m_scaned);
			UpdateData(FALSE);
			MSG msg;

			// Process existing messages in the application's message queue.
			// When the queue is empty, do clean up and return.
			while (::PeekMessage(&msg,NULL,0,0,PM_NOREMOVE))
			{
				if (!AfxGetThread()->PumpMessage())
					break;
			}
			Sleep(1);
		}

		int sbs = 0;
		int tb = 0;
		int normal = 0;
		int logo = 0;
		for(int i=0; i<nscan; i++)
		{
			if (detector.sbs_result[i] - detector.tb_result[i] > 0.1 || detector.sbs_result[i]/detector.tb_result[i]>1.2)
				sbs++;

			else if (detector.tb_result[i] - detector.sbs_result[i] > 0.1 || detector.tb_result[i]/detector.sbs_result[i]>1.2)
				tb++;

			else if (detector.tb_result[i] > 0.9 && detector.sbs_result[i] > 0.9)
			{
				//logo image with no stereo
				logo ++;
			}

			else
				normal++;
		}

		CString out;
		if (normal > sbs+tb)
		{
			if (sbs+tb == 0)
				out.Format(_T("������ӰƬ(%d%%)"), normal*100/(normal+tb+sbs));
			else
				out.Format(_T("������ӰƬ(%d%%), ����ӰƬ(%d%%), ����ӰƬ(%d%%)"), normal*100/(normal+tb+sbs), sbs*100/(sbs+tb), tb*100/(sbs+tb));
		}
		else if (sbs > tb && sbs>0)
			out.Format(_T("���Ҹ�ʽ(%d%%)"), sbs*100/(sbs+tb));
		else if (tb > sbs && tb>0)
			out.Format(_T("���¸�ʽ(%d%%)"), tb*100/(sbs+tb));
		else
			out = _T("���ʧ��");

		FILE *f = fopen("E:\\test_folder\\debug.log", "wb");
		if(f)
		{
		for(int i=0; i<nscan; i++)
			fprintf(f, "%.2f - %.2f\r\n", detector.sbs_result[i], detector.tb_result[i]);
		fclose(f);
		}
		//SetDlgItemText(IDC_STATIC1, (LPCTSTR)out);
		SetWindowText((LPCTSTR)out);

		UpdateData(FALSE);	
	}


	CDialog::OnDropFiles(hDropInfo);
}
