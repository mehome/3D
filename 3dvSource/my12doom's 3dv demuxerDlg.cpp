// my12doom's 3dv demuxerDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"

#include <WindowsX.h>
#include <DShow.h>
#include <initguid.h>
// {FBCFD6AF-B25F-4a6d-AE56-5B5303F1A40E}
DEFINE_GUID(CLSID_my12doomSource, 
			0xfbcfd6af, 0xb25f, 0x4a6d, 0xae, 0x56, 0x5b, 0x53, 0x3, 0xf1, 0xa4, 0xe);
// {1E1299A2-9D42-4F12-8791-D79E376F4143}
DEFINE_GUID(CLSID_MKVMuxer, 
			0x1E1299A2, 0x9D42, 0x4F12, 0x87, 0x91, 0xd7, 0x9e, 0x37, 0x6f, 0x41, 0x43);

#include "my12doom's 3dv demuxer.h"
#include "my12doom's 3dv demuxerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// helper functions
HRESULT GetUnconnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin)
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr))  // Already connected, not the pin we want.
			{
				pTmp->Release();
			}
			else  // Unconnected, this is the pin we want.
			{
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// Cmy12dooms3dvdemuxerDlg �Ի���




Cmy12dooms3dvdemuxerDlg::Cmy12dooms3dvdemuxerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cmy12dooms3dvdemuxerDlg::IDD, pParent),
m_thread(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cmy12dooms3dvdemuxerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress);
}

BEGIN_MESSAGE_MAP(Cmy12dooms3dvdemuxerDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_CREATE()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_BUTTON1, &Cmy12dooms3dvdemuxerDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// Cmy12dooms3dvdemuxerDlg ��Ϣ�������

BOOL Cmy12dooms3dvdemuxerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	Button_SetCheck(::GetDlgItem(m_hWnd, IDC_RADIO1), BST_CHECKED);
	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void Cmy12dooms3dvdemuxerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void Cmy12dooms3dvdemuxerDlg::OnPaint()
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
HCURSOR Cmy12dooms3dvdemuxerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


int Cmy12dooms3dvdemuxerDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void Cmy12dooms3dvdemuxerDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);
}

DWORD WINAPI Cmy12dooms3dvdemuxerDlg::worker_thread(LPVOID param)
{
	Cmy12dooms3dvdemuxerDlg * _this = (Cmy12dooms3dvdemuxerDlg*)param;
	_this->cancel = false;
	::ShowWindow(_this->m_progress.m_hWnd, SW_SHOW);
	::SetWindowText(::GetDlgItem(_this->m_hWnd, IDC_BUTTON1), _T("Cancel"));
	::ShowWindow(::GetDlgItem(_this->m_hWnd, IDC_RADIO1), SW_HIDE);
	::ShowWindow(::GetDlgItem(_this->m_hWnd, IDC_RADIO2), SW_HIDE);

	CoInitialize(NULL);
	CComPtr<IGraphBuilder> gb;
	CComPtr<IBaseFilter> source;
	CComPtr<IBaseFilter> muxer;
	CComPtr<IBaseFilter> writer;
	gb.CoCreateInstance(CLSID_FilterGraph);
	source.CoCreateInstance(CLSID_my12doomSource);
	muxer.CoCreateInstance(CLSID_MKVMuxer);
	//muxer.CoCreateInstance(CLSID_AviDest);
	writer.CoCreateInstance(CLSID_FileWriter);

	if (source == NULL)
	{
		_this->cancel_work();
		return 0;
	}

	if (muxer == NULL)
	{
		_this->cancel_work();
		return 0;
	}

	CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> source_f(source);
	CComQIPtr<IFileSinkFilter, &IID_IFileSinkFilter> writer_f(writer);
	source_f->Load(L"E:\\test_folder\\3DV������ʾ��Ƶ-������ٷ�Ԥ��Ƭ.3dv", NULL);
	//source_f->Load(L"D:\\Users\\my12doom\\Desktop\\avatar.3dv", NULL);
	//writer_f->SetFileName(L"E:\\test_folder\\243dvA.mkv", NULL);
	writer_f->SetFileName(L"D:\\Users\\my12doom\\Desktop\\avatar.mkv", NULL);
	gb->AddFilter(source, L"source");
	gb->AddFilter(muxer, L"muxer");
	gb->AddFilter(writer, L"writer");

	CComPtr<IPin> pino;
	CComPtr<IPin> pini;
	while (GetUnconnectedPin(source, PINDIR_OUTPUT, &pino) == S_OK && 
		   GetUnconnectedPin(muxer, PINDIR_INPUT, &pini) == S_OK)
	{
		HRESULT hr = gb->ConnectDirect(pino, pini, NULL);
		pino = NULL;
		pini = NULL;
	}

	pino = NULL;
	pini = NULL;

	if (GetUnconnectedPin(muxer, PINDIR_OUTPUT, &pino) == S_OK && 
		GetUnconnectedPin(writer, PINDIR_INPUT, &pini) == S_OK)
		gb->ConnectDirect(pino, pini, NULL);

	CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
	mc->Run();

	CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> ms(gb);
	CComQIPtr<IMediaEvent, &IID_IMediaEvent> me(gb);

	//OAFilterState fs = State_Running;
	REFERENCE_TIME current = 0;
	REFERENCE_TIME stop = 1;
	REFERENCE_TIME total = 1;
	long exitcode = 0;
	do 
	{
		ms->GetPositions(&current, &stop);
		ms->GetDuration(&total);
		//mc->GetState(INFINITE, &fs);

		if (total !=0)
		{
			_this->m_progress.SetRange(0, 1000);
			_this->m_progress.SetPos(current*1000/total);
		}		
	} while (me->WaitForCompletion(50, &exitcode) == E_ABORT && !_this->cancel);

	_this->cancel_work();
	return 0;
}

void Cmy12dooms3dvdemuxerDlg::cancel_work()
{
	::ShowWindow(m_progress.m_hWnd, SW_HIDE);
	::SetWindowText(::GetDlgItem(m_hWnd, IDC_BUTTON1), _T("Demux!"));
	::ShowWindow(::GetDlgItem(m_hWnd, IDC_RADIO1), SW_SHOW);
	::ShowWindow(::GetDlgItem(m_hWnd, IDC_RADIO2), SW_SHOW);
	HANDLE t = m_thread;
	m_thread = NULL;
	//TerminateThread(t, -1);
}
void Cmy12dooms3dvdemuxerDlg::OnBnClickedButton1()
{
	if (m_thread != NULL)
		cancel_work();
	else
	{
		m_thread = CreateThread(NULL, 0, worker_thread, this, NULL, NULL);
	}
}
