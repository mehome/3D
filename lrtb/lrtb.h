// lrtb.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// ClrtbApp:
// �йش����ʵ�֣������ lrtb.cpp
//

class ClrtbApp : public CWinApp
{
public:
	ClrtbApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern ClrtbApp theApp;