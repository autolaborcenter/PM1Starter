
// PM1SDKDemo.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// CPM1SDKDemoApp:
// 有关此类的实现，请参阅 PM1SDKDemo.cpp
//

class CPM1SDKDemoApp : public CWinApp
{
public:
	CPM1SDKDemoApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CPM1SDKDemoApp theApp;
