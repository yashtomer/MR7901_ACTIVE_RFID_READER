// IOCPServer.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

//#include "resource.h"		// ������


// CPiggyIOCPServerApp:
// �йش����ʵ�֣������ PiggyIOCPServer.cpp
//

class CIOCPServerApp : public CWinApp
{
public:
	CIOCPServerApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CIOCPServerApp theApp;