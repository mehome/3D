// dwindow_launcher.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error �ڰ������� PCH �Ĵ��ļ�֮ǰ������stdafx.h��
#endif

#include "resource.h"		// ������


// Cdwindow_launcherApp:
// �йش����ʵ�֣������ dwindow_launcher.cpp
//

class Cdwindow_launcherApp : public CWinApp
{
public:
	Cdwindow_launcherApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Cdwindow_launcherApp theApp;
