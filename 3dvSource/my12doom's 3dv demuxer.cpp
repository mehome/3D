// my12doom's 3dv demuxer.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "my12doom's 3dv demuxer.h"
#include "my12doom's 3dv demuxerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cmy12dooms3dvdemuxerApp

BEGIN_MESSAGE_MAP(Cmy12dooms3dvdemuxerApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// Cmy12dooms3dvdemuxerApp ����

Cmy12dooms3dvdemuxerApp::Cmy12dooms3dvdemuxerApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� Cmy12dooms3dvdemuxerApp ����

Cmy12dooms3dvdemuxerApp theApp;


// Cmy12dooms3dvdemuxerApp ��ʼ��

BOOL Cmy12dooms3dvdemuxerApp::InitInstance()
{
	CWinApp::InitInstance();

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

	Cmy12dooms3dvdemuxerDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȷ�������رնԻ���Ĵ���
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȡ�������رնԻ���Ĵ���
	}

	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}
