// interlacer_gui.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error �ڰ������� PCH �Ĵ��ļ�֮ǰ������stdafx.h��
#endif

#include "resource.h"		// ������


// Cinterlacer_guiApp:
// �йش����ʵ�֣������ interlacer_gui.cpp
//

class Cinterlacer_guiApp : public CWinApp
{
public:
	Cinterlacer_guiApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Cinterlacer_guiApp theApp;
