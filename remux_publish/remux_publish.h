// remux_publish.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error �ڰ������� PCH �Ĵ��ļ�֮ǰ������stdafx.h��
#endif

#include "resource.h"		// ������


// Cremux_publishApp:
// �йش����ʵ�֣������ remux_publish.cpp
//

class Cremux_publishApp : public CWinApp
{
public:
	Cremux_publishApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Cremux_publishApp theApp;
