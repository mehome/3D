// active_dwindow.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Cactive_dwindowApp:
// �йش����ʵ�֣������ active_dwindow.cpp
//

class Cactive_dwindowApp : public CWinApp
{
public:
	Cactive_dwindowApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Cactive_dwindowApp theApp;