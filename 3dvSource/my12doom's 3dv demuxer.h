// my12doom's 3dv demuxer.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Cmy12dooms3dvdemuxerApp:
// �йش����ʵ�֣������ my12doom's 3dv demuxer.cpp
//

class Cmy12dooms3dvdemuxerApp : public CWinApp
{
public:
	Cmy12dooms3dvdemuxerApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Cmy12dooms3dvdemuxerApp theApp;