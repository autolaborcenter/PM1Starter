
// PM1SDKDemoDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "PM1SDKDemo.h"
#include "PM1SDKDemoDlg.h"
#include "AboutDlg.h"
#include "afxdialogex.h"
#include "pm1_sdk.h"
#include <thread>

using namespace autolabor::pm1;

#pragma comment(lib, "pm1_sdk.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const double PI = 3.1415926;
const double EPSILON = 1e-5;

// 构造
CPM1SDKDemoDlg::CPM1SDKDemoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PM1SDKDEMO_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	// 参数初始值
	// 直行
	m_arrSpeed[0] = 0.2;
	m_arrForward[0] = 1;
	m_arrTime[0] = m_arrForward[0] / m_arrSpeed[0];
	// 转弯
	m_arrSpeed[1] = 0.2;
	m_arrForward[1] = 90;
	m_dRadius = 2;
	m_arrTime[1] = (2 * PI * m_dRadius * m_arrForward[1] / 360) / m_arrSpeed[1];
	// 原地转
	m_arrSpeed[2] = 10;
	m_arrForward[2] = 180;
	m_arrTime[2] = m_arrForward[2] / m_arrSpeed[2];
	// 摇杆
	m_JoystickRect.SetRect(350, 200, 650, 500);
	m_JoystickPos.SetPoint(m_JoystickRect.CenterPoint().x, m_JoystickRect.CenterPoint().y);
	m_bJoystickDown = false;
}

void CPM1SDKDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_CLOSE, m_btnClose);
	DDX_Control(pDX, IDC_BTN_MINIMIZE, m_btnMinimize);
	DDX_Control(pDX, IDC_BTN_ABOUT, m_btnAbout);
	DDX_Control(pDX, IDC_CB_COM, m_cbCom);
	DDX_Control(pDX, IDC_CB_INSTRUCTION, m_cbInstruction);
	DDX_Control(pDX, IDC_PROGRESS_GO, m_progressGo);
	DDX_Control(pDX, IDC_SLIDER_V, m_sliderV);
	DDX_Control(pDX, IDC_SLIDER_S, m_sliderS);
	DDX_Control(pDX, IDC_SLIDER_T, m_sliderT);
	DDX_Control(pDX, IDC_SLIDER_R, m_sliderR);
	DDX_Control(pDX, IDC_EDIT_V, m_editV);
	DDX_Control(pDX, IDC_EDIT_S, m_editS);
	DDX_Control(pDX, IDC_EDIT_T, m_editT);
	DDX_Control(pDX, IDC_EDIT_R, m_editR);
}

BEGIN_MESSAGE_MAP(CPM1SDKDemoDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CLOSE, &CPM1SDKDemoDlg::OnBnClickedBtnClose)
	ON_BN_CLICKED(IDC_BTN_MINIMIZE, &CPM1SDKDemoDlg::OnBnClickedBtnMinimize)
	ON_BN_CLICKED(IDC_BTN_ABOUT, &CPM1SDKDemoDlg::OnBnClickedBtnAbout)
	ON_CBN_SELCHANGE(IDC_CB_INSTRUCTION, &CPM1SDKDemoDlg::OnCbnSelchangeCbInstruction)
	ON_EN_CHANGE(IDC_EDIT_V, &CPM1SDKDemoDlg::OnEnChangeEditV)
	ON_EN_CHANGE(IDC_EDIT_S, &CPM1SDKDemoDlg::OnEnChangeEditS)
	ON_EN_CHANGE(IDC_EDIT_T, &CPM1SDKDemoDlg::OnEnChangeEditT)
	ON_EN_CHANGE(IDC_EDIT_R, &CPM1SDKDemoDlg::OnEnChangeEditR)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BTN_GO, &CPM1SDKDemoDlg::OnBnClickedBtnGo)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_BTN_INSTRUCTION, &CPM1SDKDemoDlg::OnBnClickedBtnInstruction)
	ON_BN_CLICKED(IDC_BTN_REALTIME, &CPM1SDKDemoDlg::OnBnClickedBtnRealtime)
	ON_BN_CLICKED(IDC_BTN_OPEN, &CPM1SDKDemoDlg::OnBnClickedBtnOpen)
	ON_WM_ERASEBKGND()
	ON_CBN_SELCHANGE(IDC_CB_COM, &CPM1SDKDemoDlg::OnCbnSelchangeCbCom)
	ON_WM_NCCALCSIZE()
	ON_WM_NCHITTEST()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_ABORT, &CPM1SDKDemoDlg::OnBnClickedBtnAbort)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// 初始化
BOOL CPM1SDKDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// 初始化代码
	
	// 获取标题栏高度与颜色
	CRect rect;
	GetDlgItem(IDC_PIC_TITLE)->GetWindowRect(rect);
	CImage image;
	HBITMAP hbmp = (HBITMAP)LoadImage(AfxGetInstanceHandle(), 
		MAKEINTRESOURCE(IDB_TITLE), IMAGE_BITMAP, 0, 0, 0);
	image.Attach(hbmp);
	m_nTitleHeight = image.GetHeight() + rect.top * 2 + 1;
	m_TitleColor = image.GetPixel(0, 0);
	image.Detach();
	image.Destroy();
	DeleteObject(hbmp);

	// 关闭按钮
	GetWindowRect(rect);
	CRect rect1;
	m_btnClose.LoadBitmaps(IDB_CLOSE_NORM, IDB_CLOSE_OVER, IDB_CLOSE_HIT);
	m_btnClose.GetWindowRect(rect1);
	m_btnClose.MoveWindow(rect.Width() - rect1.Width() - 1, 1, rect1.Width(), rect1.Height());
	// 最小化按钮
	CRect rect2;
	m_btnMinimize.LoadBitmaps(IDB_MINIMIZE_NORM, IDB_MINIMIZE_OVER, IDB_MINIMIZE_HIT);
	m_btnMinimize.GetWindowRect(rect2);
	m_btnMinimize.MoveWindow(rect.Width() - rect1.Width() - rect2.Width() - 1, 1, 
		rect2.Width(), rect2.Height());
	// 关于按钮
	CRect rect3;
	m_btnAbout.LoadBitmaps(IDB_ABOUT_NORM, IDB_ABOUT_OVER, IDB_ABOUT_HIT);
	m_btnAbout.GetWindowRect(rect3);
	m_btnAbout.MoveWindow(rect.Width() - rect1.Width() - rect2.Width() - rect3.Width() - 1, 1,
		rect3.Width(), rect3.Height());

	// 按钮提示
	m_Tip.Create(this);
	m_Tip.AddTool(GetDlgItem(IDC_BTN_CLOSE), L"关闭");
	m_Tip.AddTool(GetDlgItem(IDC_BTN_MINIMIZE), L"最小化");
	m_Tip.AddTool(GetDlgItem(IDC_BTN_ABOUT), L"关于");
	m_Tip.SetDelayTime(200);
	m_Tip.SetDelayTime(TTDT_AUTOPOP, 2000);
	m_Tip.Activate(TRUE);

	// 串口列表
	std::vector<std::string> names = serial_ports();
	for (int i = 0; i < names.size(); i++)
	{
		CString name(names[i].c_str());
		m_cbCom.AddString(name);
	}
	m_cbCom.AddString(L"自动选择");
	m_cbCom.AddString(L"自定义");
	m_cbCom.SetCurSel(names.size());// 默认自动选择

	// 连接提示字体
	CFont* pft = GetFont();
	LOGFONT lf;
	pft->GetLogFont(&lf);
	lf.lfHeight = LONG(1.35*lf.lfHeight);	// 加大字号
	m_tipFont.CreateFontIndirect(&lf);
	GetDlgItem(IDC_TEXT_INFO)->SetFont(&m_tipFont);

	// 设置指令类型
	m_cbInstruction.AddString(L"直行");
	m_cbInstruction.AddString(L"转弯");
	m_cbInstruction.AddString(L"原地转向");
	m_cbInstruction.SetCurSel(0);	// 默认直行
	OnCbnSelchangeCbInstruction();

	// 设置进度条
	m_progressGo.SetRange(0, 100);

	// 调整界面参数
	CRect tmpRect;
	GetDlgItem(IDC_CB_COM)->GetWindowRect(tmpRect);
	ScreenToClient(tmpRect);
	int top = tmpRect.bottom;
	GetDlgItem(IDC_BTN_INSTRUCTION)->GetWindowRect(tmpRect);
	ScreenToClient(tmpRect);
	int bottom = tmpRect.top;
	m_nLineTop = (top + bottom) / 2;
	m_nLineLeft = tmpRect.right + tmpRect.left;
	GetDlgItem(IDC_BTN_ABORT)->GetWindowRect(tmpRect);
	ScreenToClient(tmpRect);
	m_nLineBottom = tmpRect.top - 5;

	// 摇杆尺寸位置
	int d = (m_nLineBottom - m_nLineTop) / 13;
	int left = (rect.Width() - m_nLineLeft - 10 * d) / 2 + m_nLineLeft;
	top = m_nLineTop + 1.5 * d;
	m_JoystickRect.SetRect(left, top, left + 10 * d, top + 10 * d);
	m_JoystickPos.SetPoint(m_JoystickRect.CenterPoint().x, m_JoystickRect.CenterPoint().y);

	// 调整窗口大小
	MoveWindow(rect.left, rect.top, rect.Width(), tmpRect.bottom + 5);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 清理
void CPM1SDKDemoDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_tipFont.DeleteObject();
}

// 界面绘图
void CPM1SDKDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		//CDialogEx::OnPaint();
		CPaintDC dc(this); // 用于绘制的设备上下文

		CRect rect;
		GetClientRect(rect);
		// 填充标题栏
		CPen titlePen(PS_SOLID, 1, m_TitleColor);
		CPen* pOldPen = dc.SelectObject(&titlePen);
		CBrush titleBrush(m_TitleColor);
		CBrush* pOldBrush = dc.SelectObject(&titleBrush);
		dc.Rectangle(0, 0, rect.Width(), m_nTitleHeight);
		dc.SelectObject(pOldPen);
		dc.SelectObject(pOldBrush);
		titlePen.DeleteObject();
		titleBrush.DeleteObject();

		// 画窗口边框
		CPen edgePen(PS_SOLID, 1, RGB(24, 131, 215));
		dc.SelectObject(&edgePen);
		dc.MoveTo(0, 0);
		dc.LineTo(rect.Width() - 1, 0);
		dc.LineTo(rect.Width() - 1, rect.Height() - 1);
		dc.LineTo(0, rect.Height() - 1);
		dc.LineTo(0, 0);
		dc.SelectObject(pOldPen);
		edgePen.DeleteObject();

		// 窗口内部边线
		CPen linePen(PS_SOLID, 1, RGB(0, 0, 0));
		dc.SelectObject(&linePen);
		dc.MoveTo(0, m_nLineTop);
		dc.LineTo(rect.Width(), m_nLineTop);
		dc.MoveTo(m_nLineLeft, m_nLineTop);
		dc.LineTo(m_nLineLeft, m_nLineBottom);
		dc.MoveTo(0, m_nLineBottom);
		dc.LineTo(rect.Width(), m_nLineBottom);
		dc.SelectObject(pOldPen);
		linePen.DeleteObject();

		// 摇杆(双缓冲避免闪烁)
		if (m_WorkMode == WorkMode::REALTIME)
		{
			int w = m_JoystickRect.Width();
			int h = m_JoystickRect.Height();
			int r = w / 10;
			// 双缓冲
			CDC memDC;
			CBitmap memBmp;
			memDC.CreateCompatibleDC(NULL);
			memBmp.CreateCompatibleBitmap(&dc, w + 2 * r, h + 2 * r);
			memDC.SelectObject(&memBmp);
			// 擦背景
			memDC.FillSolidRect(0, 0, w + 2 * r, h + 2 * r, RGB(240, 240, 240));
			// 设置画笔
			CPen memPen(PS_SOLID, 2, RGB(0, 0, 0));
			CPen* pPen = memDC.SelectObject(&memPen);
			// 摇杆外框与中心十字
			CPoint pt(r + w / 2, r);
			CRect rect(r, r, r + w, r + h);
			memDC.Arc(rect, pt, pt);
			CPoint pt1(r + w / 2, r + h / 2);
			memDC.MoveTo(pt1.x - r, pt1.y);
			memDC.LineTo(pt1.x + r - 1, pt1.y);
			memDC.MoveTo(pt1.x, pt1.y - r);
			memDC.LineTo(pt1.x, pt1.y + r - 1);
			// 设置画笔
			memDC.SelectObject(pPen);
			CBrush memBrush(RGB(240, 240, 240));
			if (m_bExecuting)
			{
				memDC.SelectObject(&memBrush);
			}
			// 摇杆
			int x = m_JoystickPos.x - m_JoystickRect.left + r;
			int y = m_JoystickPos.y - m_JoystickRect.top + r;
			memDC.Ellipse(x - r, y - r, x + r, y + r);
			// 拷贝内存图到屏幕显示
			dc.BitBlt(m_JoystickRect.left - r, m_JoystickRect.top - r,
				w + 2 * r, h + 2 * r, &memDC, 0, 0, SRCCOPY);
			// 清理
			memBmp.DeleteObject();
			memDC.DeleteDC();
			memPen.DeleteObject();
			dc.SelectObject(pOldPen);
			dc.SelectObject(pOldBrush);
		}
	}
}

// 背景重绘
BOOL CPM1SDKDemoDlg::OnEraseBkgnd(CDC* pDC)
{
	if (m_bJoystickDown)
	{
		return TRUE; // 配合双缓冲避免摇杆闪烁
	}

	return CDialogEx::OnEraseBkgnd(pDC);
}

// 颜色设置
HBRUSH CPM1SDKDemoDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (CTLCOLOR_DLG == nCtlColor)	// 对话框背景色
	{
		//return m_hBkBrush;
	}
	else if (CTLCOLOR_STATIC == nCtlColor)	// 静态文本
	{
		if (m_bTipWarn && IDC_TEXT_INFO == pWnd->GetDlgCtrlID())
		{
			pDC->SetTextColor(RGB(180, 0, 0));	// 红色提示
			pDC->SetBkMode(TRANSPARENT);
			return (HBRUSH)GetStockObject(NULL_BRUSH);
		}
	}
	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

// 拦截WM_NCCALCSIZE
void CPM1SDKDemoDlg::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	// 拦截WM_NCCALCSIZE实现无标题栏及边框(保留动画效果)
	//CDialogEx::OnNcCalcSize(bCalcValidRects, lpncsp);
}

// 窗口拖动
LRESULT CPM1SDKDemoDlg::OnNcHitTest(CPoint point)
{
	POINT pt;
	pt.x = point.x;
	pt.y = point.y;
	ScreenToClient(&pt);
	if (pt.y <= m_nTitleHeight || pt.y >= 560)
	{
		return HTCAPTION;
	}

	return CDialogEx::OnNcHitTest(point);
}

// 屏蔽F1帮助
void CPM1SDKDemoDlg::WinHelp(DWORD dwData, UINT nCmd)
{
	//CDialog::WinHelp(dwData, nCmd);
}

// 关闭按钮
void CPM1SDKDemoDlg::OnBnClickedBtnClose()
{
	OnCancel();
}

// 最小化按钮
void CPM1SDKDemoDlg::OnBnClickedBtnMinimize()
{
	ShowWindow(SW_SHOWMINIMIZED);
}

// 关于按钮
void CPM1SDKDemoDlg::OnBnClickedBtnAbout()
{
	CAboutDlg about;
	about.DoModal();
}

// 串口选择事件(实现自定义可编辑其他选项不可编辑)
void CPM1SDKDemoDlg::OnCbnSelchangeCbCom()
{
	DWORD style = m_cbCom.GetStyle();
	if (m_cbCom.GetCurSel() == m_cbCom.GetCount() - 1 && (style | CBS_DROPDOWNLIST))
	{
		int sel = m_cbCom.GetCurSel();
		CString* items = new CString[m_cbCom.GetCount()];
		for (int i = 0; i < m_cbCom.GetCount(); i++)
		{
			m_cbCom.GetLBText(i, items[i]);
		}
		CFont* pFont = m_cbCom.GetFont();
		CRect rect;
		m_cbCom.GetWindowRect(&rect);
		ScreenToClient(&rect);
		UINT id = m_cbCom.GetDlgCtrlID();
		style = style & ~CBS_DROPDOWNLIST | CBS_DROPDOWN;
		m_cbCom.DestroyWindow();
		m_cbCom.Create(style, rect, this, id);
		m_cbCom.SetFont(pFont);
		m_cbCom.ShowWindow(SW_SHOW);
		for (int i = 0; i < items->GetLength(); i++)
		{
			m_cbCom.AddString(items[i]);
		}
		m_cbCom.SetCurSel(sel);
		delete[] items;
	}
	else if (m_cbCom.GetCurSel() != m_cbCom.GetCount() - 1 && !(style & CBS_SIMPLE))
	{
		int sel = m_cbCom.GetCurSel();
		CString* items = new CString[m_cbCom.GetCount()];
		for (int i = 0; i < m_cbCom.GetCount(); i++)
		{
			m_cbCom.GetLBText(i, items[i]);
		}
		CFont* pFont = m_cbCom.GetFont();
		CRect rect;
		m_cbCom.GetWindowRect(&rect);
		ScreenToClient(&rect);
		UINT id = m_cbCom.GetDlgCtrlID();
		DWORD style = 0;
		style = m_cbCom.GetExStyle() | CBS_DROPDOWNLIST;
		m_cbCom.DestroyWindow();
		m_cbCom.Create(style, rect, this, id);
		m_cbCom.SetFont(pFont);
		m_cbCom.ShowWindow(SW_SHOW);
		for (int i = 0; i < items->GetLength(); i++)
		{
			m_cbCom.AddString(items[i]);
		}
		m_cbCom.SetCurSel(sel);
		delete[] items;
	}
}

// 设置连接提示信息
void CPM1SDKDemoDlg::SetTipText(CString text, BOOL warn)
{
	m_bTipWarn = warn;
	GetDlgItem(IDC_TEXT_INFO)->SetWindowText(text);
	RECT rect;
	GetDlgItem(IDC_TEXT_INFO)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	InvalidateRect(&rect);
}

// 连接串口按钮
void CPM1SDKDemoDlg::OnBnClickedBtnOpen()
{
	CString sName;
	GetDlgItem(IDC_BTN_OPEN)->GetWindowText(sName);
	if (sName == L"连接")
	{
		CString sCom;
		m_cbCom.GetWindowText(sCom);
		if (sCom.Trim().GetLength() == 0 || sCom == L"自定义")
		{
			SetTipText(L"请输入串口名称", TRUE);
			return;
		}
		// 调用接口初始化
		result suc =
			sCom == L"自动选择" ?
			initialize() :
			initialize((std::string)CT2A(sCom.GetBuffer()));
		if (suc)
		{
			GetDlgItem(IDC_BTN_OPEN)->SetWindowText(L"断开连接");
			m_cbCom.EnableWindow(FALSE);
			SetTipText(sCom + L"已连接");
			GetDlgItem(IDC_BTN_GO)->EnableWindow(TRUE);
			m_bConnected = true;
			SetTimer(1, 200, NULL);	// 定时读取里程计数据
		}
		else
		{
			SetTipText(L"连接失败", TRUE);
		}
	}
	else
	{
		KillTimer(1);	// 停止取里程计数据
		shutdown();		// 调用接口断开连接
		((CButton*)GetDlgItem(IDC_BTN_OPEN))->SetWindowText(L"连接");
		m_cbCom.EnableWindow(TRUE);
		SetTipText(L"未连接");
		GetDlgItem(IDC_BTN_GO)->EnableWindow(FALSE);
		m_bConnected = false;
	}
}

// 指令模式界面显示
void CPM1SDKDemoDlg::ShowInstruction(BOOL bShow)
{
	GetDlgItem(IDC_CB_INSTRUCTION)->ShowWindow(bShow);
	for (int i = IDC_TEXT_V; i <= IDC_TEXT_R_U; i++)
	{
		if (GetDlgItem(i))
		{
			GetDlgItem(i)->ShowWindow(bShow);
		}
	}
	GetDlgItem(IDC_BTN_GO)->ShowWindow(bShow);
	if (bShow)
	{
		OnCbnSelchangeCbInstruction();
	}
}

// 指令模式按钮
void CPM1SDKDemoDlg::OnBnClickedBtnInstruction()
{
	m_WorkMode = WorkMode::INSTRUCTION;
	ShowInstruction(TRUE);
	Invalidate();
}

// 实时模式按钮
void CPM1SDKDemoDlg::OnBnClickedBtnRealtime()
{
	m_WorkMode = WorkMode::REALTIME;
	ShowInstruction(FALSE);
	Invalidate();
}

// 设置参数界面
void CPM1SDKDemoDlg::SetParamView(CString name1, CString unit1,
	CString name2, CString unit2, BOOL radius)
{
	// 参数名称与单位
	GetDlgItem(IDC_TEXT_V)->SetWindowText(name1);
	GetDlgItem(IDC_TEXT_V_U)->SetWindowText(unit1);
	GetDlgItem(IDC_TEXT_S)->SetWindowText(name2);
	GetDlgItem(IDC_TEXT_S_U)->SetWindowText(unit2);
	// 是否有半径
	GetDlgItem(IDC_TEXT_R)->ShowWindow(radius);
	GetDlgItem(IDC_SLIDER_R)->ShowWindow(radius);
	GetDlgItem(IDC_EDIT_R)->ShowWindow(radius);
	GetDlgItem(IDC_TEXT_R_U)->ShowWindow(radius);
	// 调整位置(纵向与第一行对齐,横向均匀分布)
	CRect rect[5];
	for (int i = IDC_TEXT_V; i <= IDC_TEXT_V_U; i++)
	{
		GetDlgItem(i)->GetWindowRect(rect[i - IDC_TEXT_V]);
		ScreenToClient(rect[i- IDC_TEXT_V]);
	}
	GetDlgItem(IDC_TEXT_S)->GetWindowRect(rect[4]);
	ScreenToClient(rect[4]);
	int dh = rect[4].top - rect[0].top;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 1; j < 4; j++)
		{
			GetDlgItem(IDC_TEXT_V + 10 * j + i)->MoveWindow(
				rect[i].left, rect[i].top + dh * j, rect[i].Width(), rect[i].Height());
		}
	}
	// 调整执行按钮位置
	GetDlgItem(IDC_BTN_GO)->GetWindowRect(rect[4]);
	GetDlgItem(IDC_BTN_GO)->MoveWindow(rect[2].left, rect[2].top + dh * (radius ? 4 : 3), 
		rect[4].Width(), rect[4].Height());
}

// 设置参数文本框内容(避免文本变化事件)
void CPM1SDKDemoDlg::SetEditText(int index, CString text)
{
	if (index >= 0 && index < 4)
	{
		m_arrAutoModify[index] = TRUE;
		GetDlgItem(IDC_EDIT_V + 10 * index)->SetWindowText(text);
	}
}

// 界面切换到直行指令状态
void CPM1SDKDemoDlg::SwitchToLine()
{
	// 界面
	SetParamView(L"速度", L"m/s", L"路程", L"m", FALSE);
	// 文本框
	CString sVal;
	// edit
	m_editV.SetRange(LSPEED_MIN, LSPEED_MAX);
	sVal.Format(L"%.2f", m_arrSpeed[0]);
	SetEditText(0, sVal.TrimRight('0').TrimRight('.'));
	// edit
	m_editS.SetRange(COMMOM_MIN, COMMON_MAX);
	sVal.Format(L"%.2f", m_arrForward[0]);
	SetEditText(1, sVal.TrimRight('0').TrimRight('.'));
	// edit
	m_editT.SetRange(COMMOM_MIN, COMMON_MAX);
	sVal.Format(L"%.2f", m_arrTime[0]);
	SetEditText(2, sVal.TrimRight('0').TrimRight('.'));
	// slider
	m_sliderV.SetRange(-100, 100);
	m_sliderV.SetTicFreq(1);
	m_sliderV.SetPos(m_arrSpeed[0] * 100);
	// slider
	m_sliderS.SetRange(0, 100);
	m_sliderS.SetTicFreq(1);
	m_sliderS.SetPos(m_arrForward[0] * 10);
	// slider
	m_sliderT.SetRange(0, 60);
	m_sliderT.SetTicFreq(1);
	m_sliderT.SetPos(m_arrTime[0]);
	m_nInputLegal = (m_editV.m_bInputOk ? 1 : 0) | 
		(m_editS.m_bInputOk ? 2 : 0) | (m_editT.m_bInputOk ? 4 : 0) | 8;
}

// 界面切换到转弯指令状态
void CPM1SDKDemoDlg::SwitchToArc()
{
	// 界面
	SetParamView(L"速度", L"m/s", L"角度", L"degree", TRUE);
	// 文本框
	CString sVal;
	// edit
	m_editV.SetRange(LSPEED_MIN, LSPEED_MAX);
	sVal.Format(L"%.2f", m_arrSpeed[1]);
	SetEditText(0, sVal.TrimRight('0').TrimRight('.'));
	// edit
	m_editS.SetRange(COMMOM_MIN, COMMON_MAX);
	sVal.Format(L"%.2f", m_arrForward[1]);
	SetEditText(1, sVal.TrimRight('0').TrimRight('.'));
	// edit
	m_editT.SetRange(COMMOM_MIN, COMMON_MAX);
	sVal.Format(L"%.2f", m_arrTime[1]);
	SetEditText(2, sVal.TrimRight('0').TrimRight('.'));
	// edit
	m_editR.SetRange(-COMMON_MAX, COMMON_MAX);
	sVal.Format(L"%.2f", m_dRadius);
	SetEditText(3, sVal.TrimRight('0').TrimRight('.'));
	// slider
	m_sliderV.SetRange(-100, 100);
	m_sliderV.SetTicFreq(1);
	m_sliderV.SetPos(m_arrSpeed[1] * 100);
	// slider
	m_sliderS.SetRange(0, 360);
	m_sliderS.SetTicFreq(1);
	m_sliderS.SetPos(m_arrForward[1]);
	// slider
	m_sliderT.SetRange(0, 60);
	m_sliderT.SetTicFreq(1);
	m_sliderT.SetPos(m_arrTime[1]);
	// slider
	m_sliderR.SetRange(-100, 100);
	m_sliderR.SetTicFreq(1);
	m_sliderR.SetPos(m_dRadius * 10);
	m_nInputLegal = (m_editV.m_bInputOk ? 1 : 0) | (m_editS.m_bInputOk ? 2 : 0) |
		(m_editT.m_bInputOk ? 4 : 0) | (m_editR.m_bInputOk ? 8 : 0);
}

// 界面切换到原地转弯指令状态
void CPM1SDKDemoDlg::SwitchToAround()
{
	// 界面
	SetParamView(L"角速度", L"degree/s", L"角度", L"degree", FALSE);
	// 文本框
	CString sVal;
	// edit
	m_editV.SetRange(ASPEED_MIN, ASPEED_MAX);
	sVal.Format(L"%.2f", m_arrSpeed[2]);
	SetEditText(0, sVal.TrimRight('0').TrimRight('.'));
	// edit
	m_editS.SetRange(COMMOM_MIN, COMMON_MAX);
	sVal.Format(L"%.2f", m_arrForward[2]);
	SetEditText(1, sVal.TrimRight('0').TrimRight('.'));
	// edit
	m_editT.SetRange(COMMOM_MIN, COMMON_MAX);
	sVal.Format(L"%.2f", m_arrTime[2]);
	SetEditText(2, sVal.TrimRight('0').TrimRight('.'));
	// slider
	m_sliderV.SetRange(-90, 90);
	m_sliderV.SetTicFreq(1);
	m_sliderV.SetPos(m_arrSpeed[2]);
	// slider
	m_sliderS.SetRange(0, 360);
	m_sliderS.SetTicFreq(1);
	m_sliderS.SetPos(m_arrForward[2]);
	// slider
	m_sliderT.SetRange(0, 60);
	m_sliderT.SetTicFreq(1);
	m_sliderT.SetPos(m_arrTime[2]);
	m_nInputLegal = (m_editV.m_bInputOk ? 1 : 0) |
		(m_editS.m_bInputOk ? 2 : 0) | (m_editT.m_bInputOk ? 4 : 0) | 8;
}

// 指令切换
void CPM1SDKDemoDlg::OnCbnSelchangeCbInstruction()
{
	CString sVal;
	switch (m_cbInstruction.GetCurSel()) // 指令索引
	{
	case 0:	// 直行
		SwitchToLine();
		break;
	case 1:	// 转弯
		SwitchToArc();
		break;
	case 2:	// 原地转
		SwitchToAround();
		break;
	default:
		break;
	}
	GetDlgItem(IDC_BTN_GO)->EnableWindow(m_nInputLegal == 0x0F && m_bConnected);
	Invalidate();
}

// 文本框文本变化响应
void CPM1SDKDemoDlg::OnEnChangeEditV()
{
	// 更新值
	CString text;
	m_editV.GetWindowText(text);
	double value = _ttof(text);
	int select = m_cbInstruction.GetCurSel(); // 指令索引
	if (select >= 0 && select <= INST_NUM)
	{
		m_arrSpeed[select] = value;
		// 设置滑块
		int pos = int(select == 2 ? value : value * 100);
		m_sliderV.SetPos(pos);
		// 非手动修改返回
		if (m_arrAutoModify[0])
		{
			m_arrAutoModify[0] = FALSE;
			return;
		}
		// 更新参数值
		if (m_arrSpeed[select] != 0)
		{
			if (select == 1)
			{
				m_arrTime[1] = fabs((2 * PI * m_dRadius * m_arrForward[1] / 360) / m_arrSpeed[1]);
			}
			else
			{
				m_arrTime[select] = fabs(m_arrForward[select] / m_arrSpeed[select]);
			}
			// 更新显示
			m_editT.GetWindowText(text);
			value = _ttof(text);
			if (fabs(value - m_arrTime[select]) > EPSILON)
			{
				text.Format(L"%.2f", m_arrTime[select]);
				SetEditText(2, text.TrimRight('0').TrimRight('.'));
			}
		}
	}
	m_nInputLegal = m_nInputLegal & (~0x01) | (m_editV.m_bInputOk ? 1 : 0);
	GetDlgItem(IDC_BTN_GO)->EnableWindow(m_nInputLegal == 0x0F && m_bConnected);
}

// 文本框文本变化响应
void CPM1SDKDemoDlg::OnEnChangeEditS()
{
	// 更新值
	CString text;
	m_editS.GetWindowText(text);
	double value = _ttof(text);
	int select = m_cbInstruction.GetCurSel(); // 指令索引
	if (select >= 0 && select <= INST_NUM)
	{
		m_arrForward[select] = value;
		// 设置滑块
		int pos = int(select == 0 ? value * 10 : value);
		m_sliderS.SetPos(pos);
		// 非手动修改返回
		if (m_arrAutoModify[1])
		{
			m_arrAutoModify[1] = FALSE;
			return;
		}
		// 更新参数值
		if (m_arrSpeed[select] != 0)
		{
			if (select == 1)
			{
				m_arrTime[1] = fabs((2 * PI * m_dRadius * m_arrForward[1] / 360) / m_arrSpeed[1]);
			}
			else
			{
				m_arrTime[select] = fabs(m_arrForward[select] / m_arrSpeed[select]);
			}
			// 更新显示
			m_editT.GetWindowText(text);
			value = _ttof(text);
			if (fabs(value - m_arrTime[select]) > EPSILON)
			{
				text.Format(L"%.2f", m_arrTime[select]);
				SetEditText(2, text.TrimRight('0').TrimRight('.'));
			}
		}
		// 时间参数
		m_bTimeParam = false;
	}
	m_nInputLegal = m_nInputLegal & (~0x02) | ((m_editS.m_bInputOk ? 1 : 0) << 1);
	GetDlgItem(IDC_BTN_GO)->EnableWindow(m_nInputLegal == 0x0F && m_bConnected);
}

// 文本框文本变化响应
void CPM1SDKDemoDlg::OnEnChangeEditT()
{
	// 更新值
	CString text;
	m_editT.GetWindowText(text);
	double value = _ttof(text);
	int select = m_cbInstruction.GetCurSel(); // 指令索引
	if (select >= 0 && select <= INST_NUM)
	{
		m_arrTime[select] = value;
		// 设置滑块
		int pos = int(value);
		m_sliderT.SetPos(pos);
		// 非手动修改返回
		if (m_arrAutoModify[2])
		{
			m_arrAutoModify[2] = FALSE;
			return;
		}
		// 更新参数值
		if (select == 1)
		{
			m_arrForward[1] = fabs(m_arrSpeed[1] * m_arrTime[1] * 360 / (2 * PI * m_dRadius));
		}
		else
		{
			m_arrForward[select] = fabs(m_arrSpeed[select] * m_arrTime[select]);
		}
		// 更新显示
		m_editS.GetWindowText(text);
		value = _ttof(text);
		if (fabs(value - m_arrForward[select]) > EPSILON)
		{
			text.Format(L"%.2f", m_arrForward[select]);
			SetEditText(1, text.TrimRight('0').TrimRight('.'));
		}
		// 时间参数
		m_bTimeParam = true;
	}
	m_nInputLegal = m_nInputLegal & (~0x04) | ((m_editT.m_bInputOk ? 1 : 0) << 2);
	GetDlgItem(IDC_BTN_GO)->EnableWindow(m_nInputLegal == 0x0F && m_bConnected);
}

// 文本框文本变化响应
void CPM1SDKDemoDlg::OnEnChangeEditR()
{
	// 更新值
	CString text;
	m_editR.GetWindowText(text);
	double value = _ttof(text);
	m_dRadius = value;
	// 设置滑块
	int pos = int(value * 10);
	m_sliderR.SetPos(pos);
	// 非手动修改返回
	if (m_arrAutoModify[3])
	{
		m_arrAutoModify[3] = FALSE;
		return;
	}
	// 更新参数值
	if (m_arrSpeed[1] != 0)
	{
		m_arrTime[1] = fabs((2 * PI * m_dRadius * m_arrForward[1] / 360) / m_arrSpeed[1]);
		// 更新显示
		m_editT.GetWindowText(text);
		value = _ttof(text);
		if (fabs(value - m_arrTime[1]) > EPSILON)
		{
			text.Format(L"%.2f", m_arrTime[1]);
			SetEditText(2, text.TrimRight('0').TrimRight('.'));
		}
	}
	m_nInputLegal = m_nInputLegal & (~0x08) | ((m_editR.m_bInputOk ? 1 : 0) << 3);
	GetDlgItem(IDC_BTN_GO)->EnableWindow(m_nInputLegal == 0x0F && m_bConnected);
}

// 滚动条响应事件
void CPM1SDKDemoDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CString text;
	int select = m_cbInstruction.GetCurSel(); // 指令索引
	if (&m_sliderV == (CSliderCtrl*)pScrollBar)
	{
		double pos = m_sliderV.GetPos(); //SB_THUMBTRACK
		text.Format(L"%.2f", select == 2 ? pos : pos / 100.0);
		m_editV.SetWindowText(text.TrimRight('0').TrimRight('.'));
	}
	else if (&m_sliderS == (CSliderCtrl*)pScrollBar)
	{
		double pos = m_sliderS.GetPos();
		text.Format(L"%.2f", select == 0 ? pos / 10.0 : pos);
		m_editS.SetWindowText(text.TrimRight('0').TrimRight('.'));
	}
	else if (&m_sliderT == (CSliderCtrl*)pScrollBar)
	{
		double pos = m_sliderT.GetPos();
		text.Format(L"%.2f", pos);
		m_editT.SetWindowText(text.TrimRight('0').TrimRight('.'));
	}
	else if (&m_sliderR == (CSliderCtrl*)pScrollBar)
	{
		double pos = m_sliderR.GetPos();
		text.Format(L"%.2f", pos / 10.0);
		m_editR.SetWindowText(text.TrimRight('0').TrimRight('.'));
	}

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

// 指令执行线程
DWORD WINAPI InstructionThread(LPVOID lpParm)
{
	CPM1SDKDemoDlg* pDlg = (CPM1SDKDemoDlg*)lpParm;
	pDlg->m_bExecuting = true;
	int index = pDlg->m_cbInstruction.GetCurSel(); // 指令索引
	switch (index)
	{
	case 0:	// 直行
		resume();
		if (!pDlg->m_bTimeParam)
		{
			go_straight(pDlg->m_arrSpeed[0], pDlg->m_arrForward[0]);
		}
		else
		{
			go_straight_timing(pDlg->m_arrSpeed[0], pDlg->m_arrTime[0]);
		}
		break;
	case 1:	// 转弯
		resume();
		if (!pDlg->m_bTimeParam)
		{
			go_arc(pDlg->m_arrSpeed[1], pDlg->m_dRadius, pDlg->m_arrForward[1] * PI / 180);
		}
		else
		{
			go_arc_timing(pDlg->m_arrSpeed[1], pDlg->m_dRadius, pDlg->m_arrTime[1]);
		}
		break;
	case 2:	// 原地转
		resume();
		if (!pDlg->m_bTimeParam)
		{
			turn_around(pDlg->m_arrSpeed[2] * PI / 180, pDlg->m_arrForward[2] * PI / 180);
		}
		else
		{
			turn_around_timing(pDlg->m_arrSpeed[2] * PI / 180, pDlg->m_arrTime[2]);
		}
		break;
	default:
		break;
	}
	int lower, upper, pos;
	pDlg->m_progressGo.GetRange(lower, upper);
	pos = pDlg->m_progressGo.GetPos();
	if (pos < upper)
	{
		pDlg->m_progressGo.SetPos(upper);
	}
	pDlg->m_bExecuting = false;
	return 0;
}

// 执行/暂停/恢复
void CPM1SDKDemoDlg::OnBnClickedBtnGo()
{
	CString sName;
	GetDlgItem(IDC_BTN_GO)->GetWindowText(sName);
	int index = m_cbInstruction.GetCurSel(); // 指令索引
	if (sName == L"执行")
	{
		// 启线程执行指令
		m_hInstructionThread = CreateThread(NULL, 0, InstructionThread, this, 0, NULL);
		// 定时更新进度条
		StartTimer(m_arrTime[index] * 10);
		CString sInst;
		m_cbInstruction.GetWindowText(sInst);
		GetDlgItem(IDC_TEXT_INSTRUCTION)->SetWindowText(sInst);
		GetDlgItem(IDC_BTN_GO)->SetWindowText(L"暂停");
		m_cbInstruction.EnableWindow(FALSE);
	}
	else if (sName == L"暂停")
	{
		StopTimer();
		GetDlgItem(IDC_BTN_GO)->SetWindowText(L"恢复");
		pause();
	}
	else
	{
		StartTimer(m_arrTime[index] * 10);
		GetDlgItem(IDC_BTN_GO)->SetWindowText(L"暂停");
		resume();
	}
}

// 高精度定时线程(指令进度条)
DWORD WINAPI TimerThread(LPVOID lpParm)
{
	CPM1SDKDemoDlg* pDlg = (CPM1SDKDemoDlg*)lpParm;
	auto tick = std::chrono::high_resolution_clock::now();
	while (pDlg->m_bTimer)
	{
		int lower, upper, pos;
		pDlg->m_progressGo.GetRange(lower, upper);
		pos = pDlg->m_progressGo.GetPos();
		if (pos < upper - 1)
		{
			pDlg->m_progressGo.SetPos(pos + 1);
		}
		else if (pos == upper)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			pDlg->m_progressGo.SetPos(0);
			pDlg->GetDlgItem(IDC_BTN_GO)->SetWindowText(L"执行");
			pDlg->m_cbInstruction.EnableWindow(TRUE);
			pDlg->StopTimer();
			break;
		}
		while (pDlg->m_bTimer && (std::chrono::high_resolution_clock::now()
			- tick) < std::chrono::milliseconds(pDlg->m_nTimerMs))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		tick = std::chrono::high_resolution_clock::now();
	}
	return 0;
}

// 启动高精度定时器
void CPM1SDKDemoDlg::StartTimer(int ms)
{
	m_bTimer = true;
	m_nTimerMs = ms;
	if (m_hTimerThread == NULL)
	{
		m_hTimerThread = CreateThread(NULL, 0, TimerThread, this, 0, NULL);
	}
	else
	{
		DWORD dwExitCode = 0;
		GetExitCodeThread(m_hTimerThread, &dwExitCode);
		if (dwExitCode != STILL_ACTIVE)
		{
			m_hTimerThread = CreateThread(NULL, 0, TimerThread, this, 0, NULL);
		}
	}
}

// 停止高精度定时器
void CPM1SDKDemoDlg::StopTimer()
{
	m_bTimer = false;
}

// 中止按钮
void CPM1SDKDemoDlg::OnBnClickedBtnAbort()
{
	if (m_hInstructionThread)
	{
		DWORD dwExitCode = 0;
		GetExitCodeThread(m_hInstructionThread, &dwExitCode);
		if (dwExitCode == STILL_ACTIVE)
		{
			TerminateThread(m_hInstructionThread, 0);
			m_bExecuting = false;
		}
	}
	StopTimer();
	m_progressGo.SetPos(0);
	GetDlgItem(IDC_BTN_GO)->SetWindowText(L"执行");
	m_cbInstruction.EnableWindow(TRUE);
	if (m_WorkMode == WorkMode::REALTIME)
	{
		InvalidateRect(m_JoystickRect);
	}
}

// 实时控制线程
DWORD WINAPI DriveThread(LPVOID lpParm)
{
	CPM1SDKDemoDlg* pDlg = (CPM1SDKDemoDlg*)lpParm;
	while (pDlg->m_bJoystickDown || pDlg->m_bDirectionKey)
	{
		drive(pDlg->m_dLSpeed, pDlg->m_dASpeed * PI / 180);
		Sleep(100);
	}
	return 0;
}

// 启动实时控制线程
void CPM1SDKDemoDlg::StartDrive()
{
	if (m_bConnected)
	{
		if (m_hDriveThread == NULL)
		{
			m_hDriveThread = CreateThread(NULL, 0, DriveThread, this, 0, NULL);
		}
		else
		{
			DWORD dwExitCode = 0;
			GetExitCodeThread(m_hDriveThread, &dwExitCode);
			if (dwExitCode != STILL_ACTIVE)
			{
				m_hDriveThread = CreateThread(NULL, 0, DriveThread, this, 0, NULL);
			}
		}
	}
}

// 虚拟摇杆-鼠标左键按下 
void CPM1SDKDemoDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bExecuting)
	{
		return;
	}
	int x = m_JoystickRect.CenterPoint().x;
	int y = m_JoystickRect.CenterPoint().y;
	int r = m_JoystickRect.Width() / 10;
	if (abs(point.x - x) < r && abs(point.y - y) < r)
	{
		m_bJoystickDown = true;
		StartDrive();// 启动实时控制线程
		CPoint pt(m_JoystickRect.CenterPoint());
		ClientToScreen(&pt);
		SetCursorPos(pt.x, pt.y);
		SetCursor(LoadCursor(NULL, IDC_SIZEALL));
		SetCapture();
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

// 虚拟摇杆-鼠标左键抬起 
void CPM1SDKDemoDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bExecuting)
	{
		return;
	}
	m_bJoystickDown = false;
	m_JoystickPos.SetPoint(m_JoystickRect.CenterPoint().x, m_JoystickRect.CenterPoint().y);
	m_dLSpeed = 0;
	m_dASpeed = 0;
	int r = m_JoystickRect.Width() / 10;
	CRect rect(m_JoystickRect.left - r, m_JoystickRect.top - r,
		m_JoystickRect.right + r, m_JoystickRect.bottom + r);
	InvalidateRect(rect);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	ReleaseCapture();

	CDialogEx::OnLButtonUp(nFlags, point);
}

// 虚拟摇杆-鼠标移动
void CPM1SDKDemoDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bJoystickDown)
	{
		int x = m_JoystickRect.CenterPoint().x;
		int y = m_JoystickRect.CenterPoint().y;
		double R = m_JoystickRect.Width() / 2.0;
		double dis = sqrt((point.x - x)*(point.x - x) + (point.y - y)*(point.y - y));
		if (dis > R)
		{
			point.x = x + (point.x - x) * R / dis;
			point.y = y + (point.y - y) * R / dis;
		}
		m_JoystickPos.SetPoint(point.x, point.y);
		m_dLSpeed = -2.0 * (point.y - m_JoystickRect.CenterPoint().y) /
			m_JoystickRect.Height() * LSPEED_MAX;
		m_dASpeed = -2.0 * (point.x - m_JoystickRect.CenterPoint().x) /
			m_JoystickRect.Width() * ASPEED_MAX;
		int r = m_JoystickRect.Width() / 10;
		CRect rect(m_JoystickRect.left - r, m_JoystickRect.top - r,
			m_JoystickRect.right + r, m_JoystickRect.bottom + r);
		InvalidateRect(rect);
		SetCursor(LoadCursor(NULL, IDC_SIZEALL));
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

// PreTranslateMessage
BOOL CPM1SDKDemoDlg::PreTranslateMessage(MSG* pMsg)
{
	UINT nKey = (UINT)pMsg->wParam;
	if (WM_KEYDOWN == pMsg->message)
	{
		if (VK_ESCAPE == nKey || VK_RETURN == nKey)	// 屏蔽Esc与Enter
		{
			return TRUE;
		}
		if ((VK_UP == nKey || VK_DOWN == nKey ||
			VK_LEFT == nKey || VK_RIGHT == nKey) &&	// 方向键按下
			m_WorkMode == WorkMode::REALTIME &&
			!m_bJoystickDown &&						// 未操作摇杆
			!m_bExecuting)							// 未执行指令
		{
			if (!m_bDirectionKey)
			{
				m_bDirectionKey = true;
				StartDrive();
			}
			switch (nKey)
			{
			case VK_UP:		// 上
				if (!m_bUp)
				{
					m_bUp = true;
					if (m_bDown)
					{
						m_dLSpeed = 0;
						m_JoystickPos.y = m_JoystickRect.CenterPoint().y;
					}
					else
					{
						m_dLSpeed = LSPEED_MAX / 2;
						m_JoystickPos.y = m_JoystickRect.top + m_JoystickRect.Height() / 4;
					}
					InvalidateRect(m_JoystickRect);
				}
				break;
			case VK_DOWN:	// 下
				if (!m_bDown)
				{
					m_bDown = true;
					if (m_bUp)
					{
						m_dLSpeed = 0;
						m_JoystickPos.y = m_JoystickRect.CenterPoint().y;
					}
					else
					{
						m_dLSpeed = LSPEED_MIN / 2;
						m_JoystickPos.y = m_JoystickRect.top + m_JoystickRect.Height() * 3 / 4;
					}
					InvalidateRect(m_JoystickRect);
				}
				break;
			case VK_LEFT:	// 左
				if (!m_bLeft)
				{
					m_bLeft = true;
					if (m_bRight)
					{
						m_dASpeed = 0;
						m_JoystickPos.x = m_JoystickRect.CenterPoint().x;
					}
					else
					{
						m_dASpeed = ASPEED_MAX / 2;
						m_JoystickPos.x = m_JoystickRect.left + m_JoystickRect.Width() / 4;
					}
					InvalidateRect(m_JoystickRect);
				}
				break;
			case VK_RIGHT:	// 右
				if (!m_bRight)
				{
					m_bRight = true;
					if (m_bLeft)
					{
						m_dASpeed = 0;
						m_JoystickPos.x = m_JoystickRect.CenterPoint().x;
					}
					else
					{
						m_dASpeed = ASPEED_MIN / 2;
						m_JoystickPos.x = m_JoystickRect.left + m_JoystickRect.Width() * 3 / 4;
					}
					InvalidateRect(m_JoystickRect);
				}
				break;
			default:
				break;
			}
			return TRUE;
		}
	}
	else if (WM_KEYUP == pMsg->message)
	{
		if ((VK_UP == nKey || VK_DOWN == nKey || 
			VK_LEFT == nKey || VK_RIGHT == nKey) &&	// 方向键抬起
			m_WorkMode == WorkMode::REALTIME &&
			!m_bJoystickDown &&						// 未操作摇杆
			!m_bExecuting)							// 未执行指令
		{
			switch (nKey)
			{
			case VK_UP:
				m_bUp = false;
				if (m_bDown)
				{
					m_dLSpeed = LSPEED_MIN / 2;
					m_JoystickPos.y = m_JoystickRect.top + m_JoystickRect.Height() * 3 / 4;
				}
				else
				{
					m_dLSpeed = 0;
					m_JoystickPos.y = m_JoystickRect.CenterPoint().y;
				}
				InvalidateRect(m_JoystickRect);
				break;
			case VK_DOWN:
				m_bDown = false;
				if (m_bUp)
				{
					m_dLSpeed = LSPEED_MAX / 2;
					m_JoystickPos.y = m_JoystickRect.top + m_JoystickRect.Height() / 4;
				}
				else
				{
					m_dLSpeed = 0;
					m_JoystickPos.y = m_JoystickRect.CenterPoint().y;
				}
				InvalidateRect(m_JoystickRect);
				break;
			case VK_LEFT:
				m_bLeft = false;
				if (m_bRight)
				{
					m_dASpeed = ASPEED_MIN / 2;
					m_JoystickPos.x = m_JoystickRect.left + m_JoystickRect.Width() * 3 / 4;
				}
				else
				{
					m_dASpeed = 0;
					m_JoystickPos.x = m_JoystickRect.CenterPoint().x;
				}
				InvalidateRect(m_JoystickRect);
				break;
			case VK_RIGHT:
				m_bRight = false;
				if (m_bLeft)
				{
					m_dASpeed = ASPEED_MAX / 2;
					m_JoystickPos.x = m_JoystickRect.left + m_JoystickRect.Width() / 4;
				}
				else
				{
					m_dASpeed = 0;
					m_JoystickPos.x = m_JoystickRect.CenterPoint().x;
				}
				InvalidateRect(m_JoystickRect);
				break;
			default:
				break;
			}
			m_bDirectionKey = m_bUp || m_bDown || m_bLeft || m_bRight;
			return TRUE;
		}
	}
	else if (WM_MOUSEMOVE == pMsg->message)	// 按钮提示
	{
		m_Tip.RelayEvent(pMsg);
	}

	return CDialog::PreTranslateMessage(pMsg);
}

// 定时器(取里程计数据)
void CPM1SDKDemoDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case 1:
		odometry odom = get_odometry();
		if (!isnan(odom.x) && !isnan(odom.y) && !isnan(odom.yaw))
		{
			CString text;
			text.Format(L"X：%.2f  Y：%.2f   Yaw：%.2f", odom.x, odom.y, odom.yaw);
			GetDlgItem(IDC_TEXT_ODOM)->SetWindowText(text);
		}
		break;
	default:
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}
