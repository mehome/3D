// dwindow_mode.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Cdwindow_modeApp:
// �йش����ʵ�֣������ dwindow_mode.cpp
//

class Cdwindow_modeApp : public CWinApp
{
public:
	Cdwindow_modeApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Cdwindow_modeApp theApp;