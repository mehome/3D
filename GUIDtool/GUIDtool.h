// GUIDtool.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CGUIDtoolApp:
// �йش����ʵ�֣������ GUIDtool.cpp
//

class CGUIDtoolApp : public CWinApp
{
public:
	CGUIDtoolApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CGUIDtoolApp theApp;