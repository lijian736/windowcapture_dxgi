
// WindowCapturer.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CWindowCapturerApp: 
// �йش����ʵ�֣������ WindowCapturer.cpp
//

class CWindowCapturerApp : public CWinApp
{
public:
	CWindowCapturerApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CWindowCapturerApp theApp;