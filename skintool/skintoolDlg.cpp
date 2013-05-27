// skintoolDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "skintool.h"
#include "skintoolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


typedef struct
{
	int x;
	int y;
	int cx;
	int cy;
	char filename[4096];
	HBITMAP bmp;
	int pic_cx;
	int pic_cy;
	bool loaded;
} object_entry;

object_entry *g_objects = new object_entry[4096];
int g_object_count;
int g_window_width = 800;
int g_window_height = 480;


// CskintoolDlg �Ի���




CskintoolDlg::CskintoolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CskintoolDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CskintoolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CskintoolDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DROPFILES()
	ON_WM_TIMER()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// CskintoolDlg ��Ϣ�������

BOOL CskintoolDlg::OnInitDialog()
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

void CskintoolDlg::OnPaint()
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


		RECT r;
		GetClientRect(&r);
		if (r.right - r.left != g_window_width || r.bottom - r.top != g_window_height)
		{
			RECT w;
			GetWindowRect(&w);

			int cx = g_window_width + w.right-w.left - r.right-r.left;
			int cy = g_window_height+ w.bottom-w.top - r.bottom-r.top;

			SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOMOVE);
		}

		CPaintDC dc(this);
		for(int i=0; i<g_object_count; i++)
		{
			HDC memDC = CreateCompatibleDC(dc);
			object_entry &p = g_objects[i];

			if (!p.loaded)
			{
				p.loaded = true;
				p.bmp = (HBITMAP)::LoadImageA(0, p.filename, IMAGE_BITMAP, 0 , 0 , LR_LOADFROMFILE );

				BITMAP bm = {0};
				GetObject( p.bmp, sizeof(bm), &bm );
				p.pic_cx = bm.bmWidth;
				p.pic_cy = bm.bmHeight;

				if (p.cx != bm.bmWidth || p.cy != bm.bmHeight)
				{
					CStringA tmp;
					tmp.Format("��⵽bmp�ļ��ֱ����벼���ļ��ֱ��ʲ�һ�£�����������\n"
						"����λ��(%d,%d), ���ִ�С(%dx%d)\n"
						"�ļ�����%s, �ļ��ֱ��ʣ�%dx%d",
						p.x, p.y, p.cx, p.cy,
						p.filename, bm.bmWidth, bm.bmHeight);

					MessageBoxA(m_hWnd, tmp, "����", MB_ICONWARNING);
				}
			}

			HGDIOBJ obj = SelectObject(memDC, p.bmp);
			::StretchBlt(dc, p.x, p.y, p.cx, p.cy, memDC, 0, 0, p.pic_cx, p.pic_cy, SRCCOPY);
			SelectObject(memDC, obj);

			DeleteObject(memDC);
		}


		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CskintoolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CskintoolDlg::OnDropFiles(HDROP hDropInfo)
{
	CDialog::OnDropFiles(hDropInfo);
}

void CskintoolDlg::OnTimer(UINT_PTR nIDEvent)
{
	InvalidateRect(NULL);

	CDialog::OnTimer(nIDEvent);
}

void CskintoolDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);


	char filename[MAX_PATH];
	::GetModuleFileNameA(NULL, filename, MAX_PATH);
	*((char*)strrchr(filename, '\\')+1) = NULL;
	strcat(filename, "skin.txt");

	FILE * f = fopen(filename, "rb");
	char line[4096];
	int n = 0;
	object_entry p;
	g_object_count = 0;
	int object_found = 0;
	fgets(line, 4096, f);
	g_window_width = atoi(line);
	fgets(line, 4096, f);
	g_window_height = atoi(line);

	while (fgets(line, 4096, f))
	{
		for(int i=strlen(line)-1; i>=0; i--)
		{
			if (line[i] == '\r' || line[i] == '\n')
				line[i] = NULL;
			else
				break;
		}

		if (strstr(line, ";"))
			*((char*)strstr(line, ";")) = NULL;

		if (n == 0)
			memset(&p, 0, sizeof(object_entry));
		if(n<4)
			((int*)&p)[n] = atoi(line);
		else if (n==4)
		{
			strcpy(p.filename, filename);
			*((char*)strrchr(p.filename, '\\')+1) = NULL;
			strcat(p.filename, line);

			n = -1;
			g_objects[g_object_count++] = p;
		}

		n++;
	}

	fclose(f);

	SetTimer(0, 100, NULL);}
