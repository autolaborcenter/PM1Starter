
// PM1StarterDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "PM1Starter.h"
#include "PM1StarterDlg.h"
#include "AboutDlg.h"
#include "afxdialogex.h"
#include "pm1_sdk.h"
#include <thread>

#include<iostream>

using namespace autolabor::pm1;

#ifdef _DEBUG
#pragma comment(lib, "pm1_sdk_debug.lib")
#else
#pragma comment(lib, "pm1_sdk.lib")
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const double PI = 3.1415926;
const double EPSILON = 1e-5;
const double ROTATION_TAN = 0.35;

const LPCTSTR sJoyTip1 = L"鼠标左键按住中心白色摇杆拖动控制车移动(前方始终为车头方向)";
const LPCTSTR sJoyTip2 = L"指令执行状态无法实时控制，请等待指令执行结束或者终止正在执行的指令";

#ifdef _DEBUG
bool debug = false;
#endif

CMutex progess_mutex(FALSE);

DWORD WINAPI ProgressThread(LPVOID lpParm);

// 构造
CPM1StarterDlg::CPM1StarterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PM1STARTER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// 参数初始值
	// 直行
	m_arrSpeed[0] = 0.1;
	m_arrForward[0] = 1;
	m_arrTime[0] = m_arrForward[0] / m_arrSpeed[0];
	// 转弯
	m_arrSpeed[1] = 0.1;
	m_arrForward[1] = 90;
	m_dRadius = 0.5;
	m_arrTime[1] = (2 * PI * m_dRadius * m_arrForward[1] / 360) / m_arrSpeed[1];
	// 原地转
	m_arrSpeed[2] = 15;
	m_arrForward[2] = 90;
	m_arrTime[2] = m_arrForward[2] / m_arrSpeed[2];
	// 参数控件绑定
	m_editV.m_Tag = m_arrSpeed;
	m_editS.m_Tag = m_arrForward;
	m_editT.m_Tag = m_arrTime;
	m_editR.m_Tag = &m_dRadius;
	// 摇杆
	m_JoystickRect.SetRect(350, 200, 650, 500);
	m_JoystickPos.SetPoint(m_JoystickRect.CenterPoint().x, m_JoystickRect.CenterPoint().y);
	m_bJoystickDown = false;
	// 速度slider比例
	m_dLk = CalcSliderK(-LSPEED_MAX/*LSPEED_MIN*/, LSPEED_MAX);
	m_dAk = CalcSliderK(-ASPEED_MAX/*ASPEED_MIN*/, ASPEED_MAX);
	// 窗体背景色
	m_hBkBrush = CreateSolidBrush(RGB(255, 255, 255));
	m_hTabBkBrush = CreateSolidBrush(RGB(235, 235, 235));
	// 运动示意图
	CBitmap bmp;
	for (int i = 0; i < BMP_NUM; i++)
	{
		bmp.LoadBitmap(IDB_FRONT + i);
		m_hBitmap[i] = (HBITMAP)bmp.GetSafeHandle();
		bmp.Detach();
	}
	bmp.DeleteObject();
}

void CPM1StarterDlg::DoDataExchange(CDataExchange* pDX)
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
	DDX_Control(pDX, IDC_BTN_INSTRUCTION, m_btnInst);
	DDX_Control(pDX, IDC_BTN_REALTIME, m_btnReal);
}

BEGIN_MESSAGE_MAP(CPM1StarterDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CLOSE, &CPM1StarterDlg::OnBnClickedBtnClose)
	ON_BN_CLICKED(IDC_BTN_MINIMIZE, &CPM1StarterDlg::OnBnClickedBtnMinimize)
	ON_BN_CLICKED(IDC_BTN_ABOUT, &CPM1StarterDlg::OnBnClickedBtnAbout)
	ON_CBN_SELCHANGE(IDC_CB_INSTRUCTION, &CPM1StarterDlg::OnCbnSelchangeCbInstruction)
	ON_EN_CHANGE(IDC_EDIT_V, &CPM1StarterDlg::OnEnChangeEditV)
	ON_EN_CHANGE(IDC_EDIT_S, &CPM1StarterDlg::OnEnChangeEditS)
	ON_EN_CHANGE(IDC_EDIT_T, &CPM1StarterDlg::OnEnChangeEditT)
	ON_EN_CHANGE(IDC_EDIT_R, &CPM1StarterDlg::OnEnChangeEditR)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BTN_GO, &CPM1StarterDlg::OnBnClickedBtnGo)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_BTN_INSTRUCTION, &CPM1StarterDlg::OnBnClickedBtnInstruction)
	ON_BN_CLICKED(IDC_BTN_REALTIME, &CPM1StarterDlg::OnBnClickedBtnRealtime)
	ON_BN_CLICKED(IDC_BTN_OPEN, &CPM1StarterDlg::OnBnClickedBtnOpen)
	ON_WM_ERASEBKGND()
	ON_CBN_SELCHANGE(IDC_CB_COM, &CPM1StarterDlg::OnCbnSelchangeCbCom)
	ON_WM_NCHITTEST()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_ABORT, &CPM1StarterDlg::OnBnClickedBtnAbort)
	ON_WM_TIMER()
	ON_CBN_DROPDOWN(IDC_CB_COM, &CPM1StarterDlg::OnCbnDropdownCbCom)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_CLEAR, &CPM1StarterDlg::OnNMClickSyslinkClear)
	ON_STN_CLICKED(IDC_PIC_STATE, &CPM1StarterDlg::OnStnClickedPicState)
	ON_STN_CLICKED(IDC_PIC_TIP, &CPM1StarterDlg::OnStnClickedPicTip)
	ON_STN_DBLCLK(IDC_PIC_TIP, &CPM1StarterDlg::OnStnDblclickPicTip)
	ON_WM_SIZE()
	ON_WM_NCLBUTTONDBLCLK()
	ON_WM_NCMOUSEMOVE()
	ON_WM_VSCROLL()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

// 初始化
BOOL CPM1StarterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// 初始化代码

	// 设置窗口属性(实现无边框情况下点击任务栏图标能够最小化与恢复)
	ModifyStyle(0, WS_MINIMIZEBOX);
	// 设置标题栏文字(用于xp下任务栏显示)
	SetWindowText(APP_NAME);

	// 获取标题栏高度与颜色
	CRect rect;
	GetDlgItem(IDC_PIC_TITLE)->GetWindowRect(rect);
	CImage image;
	HBITMAP hbmp = (HBITMAP)LoadImage(AfxGetInstanceHandle(), 
		MAKEINTRESOURCE(IDB_TITLE), IMAGE_BITMAP, 0, 0, 0);
	image.Attach(hbmp);
	m_nTitleHeight = image.GetHeight() + /*rect.top * 2 +*/ 1;
	m_TitleColor = image.GetPixel(0, 0);
	image.Detach();
	image.Destroy();
	DeleteObject(hbmp);

	CRect winRect;
	GetWindowRect(winRect);
	// 调整窗体宽度
	GetDlgItem(IDC_BTN_ABORT)->GetWindowRect(rect);
	ScreenToClient(rect);
	MoveWindow(winRect.left, winRect.top, rect.right + 17, winRect.Height());
	GetWindowRect(winRect);

	// 关闭按钮
	CRect rect1;
	m_btnClose.LoadBitmaps(IDB_CLOSE_NORM, IDB_CLOSE_OVER, IDB_CLOSE_HIT);
	m_btnClose.GetWindowRect(rect1);
	m_btnClose.MoveWindow(winRect.Width() - rect1.Width() - 1, 1, rect1.Width(), rect1.Height());
	// 最小化按钮
	CRect rect2;
	m_btnMinimize.LoadBitmaps(IDB_MINIMIZE_NORM, IDB_MINIMIZE_OVER, IDB_MINIMIZE_HIT);
	m_btnMinimize.GetWindowRect(rect2);
	m_btnMinimize.MoveWindow(winRect.Width() - rect1.Width() - rect2.Width() - 1, 1, 
		rect2.Width(), rect2.Height());
	// 关于按钮
	CRect rect3;
	m_btnAbout.LoadBitmaps(IDB_ABOUT_NORM, IDB_ABOUT_OVER, IDB_ABOUT_HIT);
	m_btnAbout.GetWindowRect(rect3);
	m_btnAbout.MoveWindow(winRect.Width() - rect1.Width() - rect2.Width() - rect3.Width() - 1, 1,
		rect3.Width(), rect3.Height());

	// 按钮提示
	m_Tip.Create(this);
	m_Tip.AddTool(GetDlgItem(IDC_BTN_CLOSE), L"关闭");
	m_Tip.AddTool(GetDlgItem(IDC_BTN_MINIMIZE), L"最小化");
	m_Tip.AddTool(GetDlgItem(IDC_BTN_ABOUT), L"关于");
	m_Tip.AddTool(GetDlgItem(IDC_PIC_STATE), L"未连接");
	m_Tip.AddTool(GetDlgItem(IDC_PIC_TIP), L"点击切换运动方向");
	m_Tip.SetDelayTime(100);
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
	// 里程计字体
	pft->GetLogFont(&lf);
	lf.lfHeight = LONG(1.1*lf.lfHeight);	// 加大字号
	m_odomFont.CreateFontIndirect(&lf);
	GetDlgItem(IDC_STATIC_ODOM_TITLE)->SetFont(&m_odomFont);
	GetDlgItem(IDC_TEXT_ODOM)->SetFont(&m_odomFont);
	// 摇杆字体
	pft->GetLogFont(&lf);
	lf.lfHeight = LONG(0.95*lf.lfHeight);	// 加大字号
	m_joyFont.CreateFontIndirect(&lf);

	// 设置指令类型
	m_cbInstruction.AddString(L"直行");
	m_cbInstruction.AddString(L"转弯");
	m_cbInstruction.AddString(L"原地转向");
	m_cbInstruction.SetCurSel(0);	// 默认直行
	OnCbnSelchangeCbInstruction();

	// 设置进度条
	m_progressGo.SetRange(0, 100);
	m_progressGo.SetBkColor(RGB(0, 0, 255));//背景色为蓝色
	m_progressGo.SetBarColor(RGB(255, 0, 0));//前景色为红色

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
	m_nLineBottom = tmpRect.top - 10;

	// 调整窗口大小
	MoveWindow(winRect.left, winRect.top, winRect.Width(), tmpRect.bottom + 10);
	GetWindowRect(winRect);

	// 设置摇杆提示信息
	GetDlgItem(IDC_TEXT_JOY_INFO)->SetWindowText(sJoyTip1);
	GetDlgItem(IDC_TEXT_JOY_INFO)->GetWindowRect(tmpRect);
	GetDlgItem(IDC_TEXT_JOY_INFO)->MoveWindow(m_nLineLeft + 1, m_nLineTop + 5,
		winRect.Width() - m_nLineLeft - 2, tmpRect.Height());

	// 摇杆尺寸位置
	int d = (m_nLineBottom - tmpRect.bottom) / 13;
	int left = (winRect.Width() - m_nLineLeft - 10 * d) / 2 + m_nLineLeft;
	top = tmpRect.bottom + 1.5 * d;
	m_JoystickRect.SetRect(left, top, left + 10 * d, top + 10 * d);
	m_JoystickPos.SetPoint(m_JoystickRect.CenterPoint().x, m_JoystickRect.CenterPoint().y);

	// 里程计行标题
	GetDlgItem(IDC_STATIC_ODOM_TITLE)->GetWindowRect(tmpRect);
	GetDlgItem(IDC_STATIC_ODOM_TITLE)->MoveWindow(10, winRect.Height() - tmpRect.Height() - 5, tmpRect.Width(), tmpRect.Height());
	GetDlgItem(IDC_STATIC_ODOM_TITLE)->GetWindowRect(tmpRect);
	ScreenToClient(tmpRect);
	// 里程计值
	GetDlgItem(IDC_TEXT_ODOM)->GetWindowRect(rect);
	GetDlgItem(IDC_TEXT_ODOM)->MoveWindow(tmpRect.right, tmpRect.top, m_nLineLeft - tmpRect.right, rect.Height());
	// "里程计"
	GetDlgItem(IDC_STATIC_ODOM)->GetWindowRect(rect);
	GetDlgItem(IDC_STATIC_ODOM)->MoveWindow(tmpRect.left, tmpRect.top - rect.Height() - 10, rect.Width(), rect.Height());
	GetDlgItem(IDC_STATIC_ODOM)->GetWindowRect(tmpRect);
	ScreenToClient(tmpRect);
	// "清零"
	GetDlgItem(IDC_SYSLINK_CLEAR)->GetWindowRect(rect);
	GetDlgItem(IDC_SYSLINK_CLEAR)->MoveWindow(tmpRect.Width() + 25, tmpRect.top, rect.Width(), rect.Height());

	// 指令模式按钮
	m_btnInst.LoadBitmaps(IDB_INST_NORM, IDB_INST_OVER, IDB_INST_HIT);
	m_btnInst.GetWindowRect(rect);
	m_btnInst.MoveWindow(1, m_nLineTop + 1, rect.Width(), rect.Height());
	// 实时模式按钮
	m_btnReal.LoadBitmaps(IDB_REAL_NORM, IDB_REAL_OVER, IDB_REAL_HIT);
	m_btnReal.MoveWindow(1, m_nLineTop + rect.Height() + 1, rect.Width(), rect.Height());
	// 默认指令模式
	m_btnInst.Select(true);

	// 状态图标位置
	GetDlgItem(IDC_PIC_STATE)->GetWindowRect(rect);
	GetDlgItem(IDC_PIC_STATE)->MoveWindow(winRect.Width() - rect.Width() - 15, 
		m_nLineTop - rect.Height() - 15, rect.Width(), rect.Height());

	// 最大速度
	GetDlgItem(IDC_SLIDER_V_MAX)->ShowWindow(FALSE);
	GetDlgItem(IDC_SLIDER_W_MAX)->ShowWindow(FALSE);
	GetDlgItem(IDC_TEXT_V_MAX)->ShowWindow(FALSE);
	GetDlgItem(IDC_TEXT_W_MAX)->ShowWindow(FALSE);
#ifdef _DEBUG
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_V_MAX))->SetRange(0, 20);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_V_MAX))->SetPos(20 - 3);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_W_MAX))->SetRange(0, 50);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_W_MAX))->SetPos(50 - 20);
#endif

	// 支持触控(解决触摸时不能及时触发按下事件的问题(移动或抬起时才触发))
	RegisterTouchWindow(TRUE, TWF_WANTPALM);

	m_bInit = true;
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 清理
void CPM1StarterDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_tipFont.DeleteObject();
	m_odomFont.DeleteObject();
	m_joyFont.DeleteObject();
	DeleteObject(m_hBkBrush);
	DeleteObject(m_hTabBkBrush);
	for (int i = 0; i < BMP_NUM; i++)
	{
		DeleteObject(m_hBitmap[i]);
	}
}

// 界面绘图
void CPM1StarterDlg::OnPaint()
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

		// 窗口左侧选项栏填充
		dc.SelectObject(CBrush::FromHandle(m_hTabBkBrush));
		dc.Rectangle(0, m_nLineTop, m_nLineLeft + 1, rect.Height());
		dc.SelectObject(pOldBrush);

		// 窗口内部边线
		CPen linePen(PS_SOLID, 1, RGB(220, 220, 220));
		dc.SelectObject(&linePen);
		dc.MoveTo(0, m_nLineTop);
		dc.LineTo(rect.Width(), m_nLineTop);
		dc.MoveTo(m_nLineLeft, m_nLineTop);
		dc.LineTo(m_nLineLeft, rect.Height());
		dc.MoveTo(m_nLineLeft, m_nLineBottom);
		dc.LineTo(rect.Width(), m_nLineBottom);
		dc.SelectObject(pOldPen);
		linePen.DeleteObject();

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
			// 绘制摇杆
			DrawJoystick(&memDC);
			// 拷贝内存图到屏幕显示
			dc.BitBlt(m_JoystickRect.left - r, m_JoystickRect.top - r,
				w + 2 * r, h + 2 * r, &memDC, 0, 0, SRCCOPY);
			// 清理
			memBmp.DeleteObject();
			memDC.DeleteDC();
			dc.SelectObject(pOldPen);
			dc.SelectObject(pOldBrush);
		}
	}
}

// 绘制摇杆
void CPM1StarterDlg::DrawJoystick(CDC* pDC)
{
	int w = m_JoystickRect.Width();
	int h = m_JoystickRect.Height();
	int r = w / 10;
	// 擦绘图背景
	CRect bkRect(0, 0, w + 2 * r, h + 2 * r);
	pDC->FillRect(bkRect, CBrush::FromHandle(m_hBkBrush));
	// 分区背景
	pDC->SelectStockObject(NULL_PEN);	// 不显示边缘
	CBrush leftBrush(RGB(255, 215, 0)); // 画刷
	CBrush* pOldBrush = pDC->SelectObject(&leftBrush);
	pDC->Pie(r, r, r + w, r + h,
		r, r + h / 2 - w / 2 * ROTATION_TAN, r, r + h / 2 + w / 2 * ROTATION_TAN);			// 左
	pDC->Pie(r, r, r + w, r + h,
		r + w, r + h / 2 + w / 2 * ROTATION_TAN, r + w, r + h / 2 - w / 2 * ROTATION_TAN);	// 右
	CBrush upBrush(RGB(253, 99, 71)); // 画刷
	pDC->SelectObject(&upBrush);
	pDC->Pie(r, r, r + w, r + h,
		r + w, r + h / 2 - w / 2 * ROTATION_TAN, r, r + h / 2 - w / 2 * ROTATION_TAN);		// 上
	CBrush downBrush(RGB(253, 245, 230)); // 画刷
	pDC->SelectObject(&downBrush);
	pDC->Pie(r, r, r + w, r + h,
		r, r + h / 2 + w / 2 * ROTATION_TAN, r + w, r + h / 2 + w / 2 * ROTATION_TAN);		// 下
	//memDC.Ellipse(r + w / 2 - r / 2, r + h / 2 - r / 2, r + w / 2 + r / 2, r + h / 2 + r / 2);
	CPen aPen(PS_SOLID, 1, RGB(128, 128, 128));
	CPen* pOldPen = pDC->SelectObject(&aPen);
	// 外框与中心
	CPoint pt(r + w / 2, r);
	//memDC.Arc(winRect, pt, pt);
	CPoint pt1(r + w / 2, r + h / 2);
	pDC->MoveTo(pt1.x - r / 3, pt1.y);
	pDC->LineTo(pt1.x + r / 3, pt1.y);
	pDC->MoveTo(pt1.x, pt1.y - r / 3);
	pDC->LineTo(pt1.x, pt1.y + r / 3);
	// 文字
	pDC->SetTextColor(RGB(50, 50, 50));
	pDC->SetBkMode(TRANSPARENT);
	pDC->SelectObject(m_joyFont);
	CRect rect(r + 5, r + h / 2 - 10, r + w / 2, r + h / 2 + 10);
	pDC->DrawText(L"逆时针原地转", rect, DT_VCENTER | DT_SINGLELINE);
	rect.SetRect(r + w / 2, r + h / 2 - 10, r + w - 5, r + h / 2 + 10);
	pDC->DrawText(L"顺时针原地转", rect, DT_VCENTER | DT_SINGLELINE | DT_RIGHT);
	rect.SetRect(r, r + h / 8 - 10, r + w, r + h / 8 + 10);
	pDC->DrawText(L"前", rect, DT_VCENTER | DT_SINGLELINE | DT_CENTER);
	rect.SetRect(r + w / 5, r + h / 4 - 10, r + w / 2, r + h / 4 + 10);
	pDC->DrawText(L"左前", rect, DT_VCENTER | DT_SINGLELINE);
	rect.SetRect(r + w / 2, r + h / 4 - 10, r + w - w / 5, r + h / 4 + 10);
	pDC->DrawText(L"右前", rect, DT_VCENTER | DT_SINGLELINE | DT_RIGHT);
	rect.SetRect(r, r + h - h / 8 - 10, r + w, r + h - h / 8 + 10);
	pDC->DrawText(L"后", rect, DT_VCENTER | DT_SINGLELINE | DT_CENTER);
	rect.SetRect(r + w / 5, r + h - h / 4 - 10, r + w / 2, r + h - h / 4 + 10);
	pDC->DrawText(L"左后", rect, DT_VCENTER | DT_SINGLELINE);
	rect.SetRect(r + w / 2, r + h - h / 4 - 10, r + w - w / 5, r + h - h / 4 + 10);
	pDC->DrawText(L"右后", rect, DT_VCENTER | DT_SINGLELINE | DT_RIGHT);
	pDC->SetBkMode(OPAQUE);
	// 摇杆
	CBrush joyBrush(RGB(255, 255, 255)); // 画刷
	CBrush joyBrushDisable(RGB(225, 225, 225)); // 画刷
	if (m_bExecuting)
	{
		pDC->SelectObject(&joyBrushDisable);
	}
	else
	{
		pDC->SelectObject(&joyBrush);
	}
	int x = m_JoystickPos.x - m_JoystickRect.left + r;
	int y = m_JoystickPos.y - m_JoystickRect.top + r;
	pDC->Ellipse(x - r, y - r, x + r, y + r);
	// 恢复清理
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
	leftBrush.DeleteObject();
	upBrush.DeleteObject();
	downBrush.DeleteObject();
	joyBrush.DeleteObject();
	joyBrushDisable.DeleteObject();
	aPen.DeleteObject();
}

// 颜色设置
HBRUSH CPM1StarterDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (CTLCOLOR_DLG == nCtlColor)	// 对话框背景色
	{
		return m_hBkBrush;
	}
	else if (CTLCOLOR_STATIC == nCtlColor)	// 静态文本
	{
		int ctrlID = pWnd->GetDlgCtrlID();
		if (m_bTipWarn && IDC_TEXT_INFO == ctrlID)
		{
			pDC->SetTextColor(RGB(180, 0, 0));	// 红色提示
		}
		else if (IDC_TEXT_JOY_INFO == ctrlID)
		{
			pDC->SetTextColor(m_bExecuting ?  RGB(180, 0, 0) : RGB(50, 50, 50));
		}
		else if ((ctrlID >= IDC_STATIC_ODOM && ctrlID <= IDC_TEXT_ODOM) 
#ifdef _DEBUG
			|| ctrlID == IDC_TEXT_V_MAX || ctrlID == IDC_TEXT_W_MAX
#endif
			)
		{
			pDC->SetBkMode(TRANSPARENT);
			return m_hTabBkBrush;
		}
		return m_hBkBrush;
	}
	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

// 非客户区检测
LRESULT CPM1StarterDlg::OnNcHitTest(CPoint point)
{
	POINT pt;
	pt.x = point.x;
	pt.y = point.y;
	ScreenToClient(&pt);
	CRect rect;
	GetClientRect(rect);
	int width = rect.Width();
	int height = rect.Height();
	int border = 3;
	if (!IsZoomed())
	{
		if (pt.y <= border && pt.x <= border)
		{
			return HTTOPLEFT;
		}
		else if (pt.y <= border && pt.x >= width - border)
		{
			return HTTOPRIGHT;
		}
		else if (pt.y >= height - border && pt.x >= width - border)
		{
			return HTBOTTOMRIGHT;
		}
		else if (pt.y >= height - border && pt.x <= border)
		{
			return HTBOTTOMLEFT;
		}
		else if (pt.y <= border)
		{
			return HTTOP;
		}
		else if (pt.y >= height - border)
		{
			return HTBOTTOM;
		}
		else if (pt.x <= border)
		{
			return HTLEFT;
		}
		else if (pt.x >= width - border)
		{
			return HTRIGHT;
		}
	}
	if (pt.y < m_nLineTop && pt.y > border && 
		pt.x > border && pt.x < width - border)
	{
		return HTCAPTION;
	}

	return CDialogEx::OnNcHitTest(point);
}

// 非客户区双击
void CPM1StarterDlg::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
	POINT pt;
	pt.x = point.x;
	pt.y = point.y;
	ScreenToClient(&pt);
	if (nHitTest == HTCAPTION)
	{
		if (IsZoomed())
		{
			ShowWindow(SW_SHOWNORMAL);
		}
		else
		{
			ShowWindow(SW_SHOWMAXIMIZED);
			CRect rect;
			SystemParametersInfo(SPI_GETWORKAREA, 0, rect, 0);
			MoveWindow(rect);
		}
		return;
	}

	CDialogEx::OnNcLButtonDblClk(nHitTest, point);
}

// 非客户区左键抬起
void CPM1StarterDlg::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	if (nHitTest == HTTOPLEFT)
	{
		SendMessage(WM_SYSCOMMAND, SC_SIZE | WMSZ_TOPLEFT, MAKELPARAM(point.x, point.y));
	}
	else if (nHitTest == HTTOPRIGHT)
	{
		SendMessage(WM_SYSCOMMAND, SC_SIZE | WMSZ_TOPRIGHT, MAKELPARAM(point.x, point.y));
	}
	else if (nHitTest == HTBOTTOMRIGHT)
	{
		SendMessage(WM_SYSCOMMAND, SC_SIZE | WMSZ_BOTTOMRIGHT, MAKELPARAM(point.x, point.y));
	}
	else if (nHitTest == HTBOTTOMLEFT)
	{
		SendMessage(WM_SYSCOMMAND, SC_SIZE | WMSZ_BOTTOMLEFT, MAKELPARAM(point.x, point.y));
	}
	else if (nHitTest == HTTOP)
	{
		SendMessage(WM_SYSCOMMAND, SC_SIZE | WMSZ_TOP, MAKELPARAM(point.x, point.y));
	}
	else if (nHitTest == HTBOTTOM)
	{
		SendMessage(WM_SYSCOMMAND, SC_SIZE | WMSZ_BOTTOM, MAKELPARAM(point.x, point.y));
	}
	else if (nHitTest == HTLEFT)
	{
		SendMessage(WM_SYSCOMMAND, SC_SIZE | WMSZ_LEFT, MAKELPARAM(point.x, point.y));
	}
	else if (nHitTest == HTRIGHT)
	{
		SendMessage(WM_SYSCOMMAND, SC_SIZE | WMSZ_RIGHT, MAKELPARAM(point.x, point.y));
	}

	CDialogEx::OnNcLButtonDown(nHitTest, point);
}

// 屏蔽F1帮助
void CPM1StarterDlg::WinHelp(DWORD dwData, UINT nCmd)
{
	//CDialog::WinHelp(dwData, nCmd);
}

// 关闭按钮
void CPM1StarterDlg::OnBnClickedBtnClose()
{
	OnCancel();
}

// 最小化按钮
void CPM1StarterDlg::OnBnClickedBtnMinimize()
{
	ShowWindow(SW_SHOWMINIMIZED);
}

// 关于按钮
void CPM1StarterDlg::OnBnClickedBtnAbout()
{
	CAboutDlg about;
	about.DoModal();
}

// 串口选择事件(实现自定义可编辑其他选项不可编辑)
void CPM1StarterDlg::OnCbnSelchangeCbCom()
{
	DWORD style = m_cbCom.GetStyle();
	if (m_cbCom.GetCurSel() == m_cbCom.GetCount() - 1 && (style | CBS_DROPDOWNLIST))
	{
		int sel = m_cbCom.GetCurSel();
		int cnt = m_cbCom.GetCount();
		CString* items = new CString[cnt];
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
		for (int i = 0; i < cnt; i++)
		{
			m_cbCom.AddString(items[i]);
		}
		m_cbCom.SetCurSel(sel);
		delete[] items;
	}
	else if (m_cbCom.GetCurSel() != m_cbCom.GetCount() - 1 && !(style & CBS_SIMPLE))
	{
		int sel = m_cbCom.GetCurSel();
		int cnt = m_cbCom.GetCount();
		CString* items = new CString[cnt];
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
		for (int i = 0; i < cnt; i++)
		{
			m_cbCom.AddString(items[i]);
		}
		m_cbCom.SetCurSel(sel);
		delete[] items;
	}
}

// 串口下拉时刷新列表
void CPM1StarterDlg::OnCbnDropdownCbCom()
{
	// 记录选中项
	CString selName;
	int sel = m_cbCom.GetCurSel();
	if (sel >= 0 && sel < m_cbCom.GetCount() - 1)
	{
		m_cbCom.GetLBText(sel, selName);
	}
	else
	{
		m_cbCom.GetWindowText(selName);
		sel = -1;
	}
	m_cbCom.ResetContent();
	// 串口列表
	std::vector<std::string> names = serial_ports();
	for (int i = 0; i < names.size(); i++)
	{
		CString name(names[i].c_str());
		m_cbCom.AddString(name);
	}
	m_cbCom.AddString(L"自动选择");
	// 恢复选中项
	if (sel >= 0)
	{
		m_cbCom.AddString(L"自定义");
		int select = names.size();
		for (int i = 0; i < m_cbCom.GetCount(); i++)
		{
			CString name;
			m_cbCom.GetLBText(i, name);
			if (name == selName)
			{
				select = i;
				break;
			}
		}
		m_cbCom.SetCurSel(select);
	}
	else
	{
		m_cbCom.AddString(selName);
		m_cbCom.SetCurSel(names.size() + 1);
	}
	
}

// 设置连接提示信息
void CPM1StarterDlg::SetTipText(CString text, BOOL warn)
{
	m_bTipWarn = warn;
	GetDlgItem(IDC_TEXT_INFO)->SetWindowText(text);
	RECT rect;
	GetDlgItem(IDC_TEXT_INFO)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	InvalidateRect(&rect);
}

// 连接
void CPM1StarterDlg::Connect()
{
	CString sCom;
	m_cbCom.GetWindowText(sCom);
	if (sCom.Trim().GetLength() == 0 || sCom == L"自定义")
	{
		SetTipText(L"请输入串口名称", TRUE);
	}
	else
	{
		SetTipText(L"正在连接，请稍候……", FALSE);
		GetDlgItem(IDC_CB_COM)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_ABORT)->EnableWindow(FALSE);
		CString str;
		GetDlgItem(IDC_TEXT_INSTRUCTION)->GetWindowText(str);
		GetDlgItem(IDC_TEXT_INSTRUCTION)->SetWindowText(L"连接");
		std::thread thread(ProgressThread, this);		// 启动进度条
		// 调用接口初始化
		auto suc =
			sCom == L"自动选择" ?
			initialize((std::string)CT2A(L""), &m_dProgress) :
			initialize((std::string)CT2A(sCom.GetBuffer()), &m_dProgress);
		m_bProgress = false;
		thread.join();	// 等待进度条结束
		GetDlgItem(IDC_BTN_ABORT)->EnableWindow(TRUE);
		GetDlgItem(IDC_TEXT_INSTRUCTION)->SetWindowText(str);
#ifdef _DEBUG
		debug = sCom.MakeUpper() == L"TEST";
#endif
		if (suc
#ifdef _DEBUG
			|| debug
#endif
			)
		{
			//set_parameter(parameter_id::length, 0.365);
			//set_parameter(parameter_id::width, 0.444);
			//set_parameter(parameter_id::wheel_radius, 0.101);
			unlock();
			GetDlgItem(IDC_BTN_OPEN)->SetWindowText(L"断开连接");
			m_cbCom.EnableWindow(FALSE);
			SetTipText(L"已连接到PM1(" + CString(suc.value.c_str()) + L")");
			GetDlgItem(IDC_BTN_GO)->EnableWindow(m_bInputLegal);
			GetDlgItem(IDC_SYSLINK_CLEAR)->EnableWindow(TRUE);
			m_bConnected = true;
			SetState(PM1State::CONNECT);
#ifdef _DEBUG
			if (!debug)
			{
				SetTimer(1, 100, NULL);	// 定时读取里程计数据
			}
#else
			SetTimer(1, 100, NULL);	// 定时读取里程计数据
#endif
		}
		else
		{
			if (sCom == L"自动选择")
			{
				SetTipText(L"已扫描所有串口，未检测到PM1!", TRUE);
			}
			else if (suc.error_info == "it's not a pm1 chassis")
			{
				SetTipText(L"未检测到PM1，请检查连接或串口号!", TRUE);
			}
			else
			{
				SetTipText(L"打开串口失败！", TRUE);
			}
			GetDlgItem(IDC_CB_COM)->EnableWindow(TRUE);
		}
	}
}

// 断开连接
void CPM1StarterDlg::Disconnect()
{
	KillTimer(1);	// 停止取里程计数据
	OnBnClickedBtnAbort(); // 终止正在执行的指令
	shutdown();			// 调用接口断开连接
	((CButton*)GetDlgItem(IDC_BTN_OPEN))->SetWindowText(L"连接");
	m_cbCom.EnableWindow(TRUE);
	SetTipText(L"未连接");
	GetDlgItem(IDC_BTN_GO)->EnableWindow(FALSE);
	GetDlgItem(IDC_SYSLINK_CLEAR)->EnableWindow(FALSE);
	GetDlgItem(IDC_CB_COM)->EnableWindow(TRUE);
	m_bConnected = false;
	SetState(PM1State::UNCONNECT);
}

// 连接线程
DWORD WINAPI ConnectThread(LPVOID lpParm)
{
	CPM1StarterDlg* pDlg = (CPM1StarterDlg*)lpParm;
	pDlg->GetDlgItem(IDC_BTN_OPEN)->EnableWindow(FALSE);
	CString sName;
	pDlg->GetDlgItem(IDC_BTN_OPEN)->GetWindowText(sName);
	if (sName == L"连接")
	{
		pDlg->Connect();
	}
	else
	{
		pDlg->Disconnect();
	}
	pDlg->GetDlgItem(IDC_BTN_OPEN)->EnableWindow(TRUE);
	return 0;
}

// 连接按钮
void CPM1StarterDlg::OnBnClickedBtnOpen()
{
	if (m_hConnectThread == NULL)
	{
		m_hConnectThread = CreateThread(NULL, 0, ConnectThread, this, 0, NULL);
	}
	else
	{
		DWORD dwExitCode = 0;
		GetExitCodeThread(m_hConnectThread, &dwExitCode);
		if (dwExitCode != STILL_ACTIVE)
		{
			m_hConnectThread = CreateThread(NULL, 0, ConnectThread, this, 0, NULL);
		}

	}
}

// 指令模式界面显示
void CPM1StarterDlg::ShowInstruction(BOOL bShow)
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
	GetDlgItem(IDC_TEXT_JOY_INFO)->ShowWindow(!bShow);
	GetDlgItem(IDC_PIC_TIP)->ShowWindow(bShow);
	if (bShow)
	{
		OnCbnSelchangeCbInstruction();
	}
	else
	{
		CRect rect;
		GetWindowRect(rect);
		rect.SetRect(m_nLineLeft, m_nLineTop, rect.Width(), m_nLineBottom);
		InvalidateRect(rect);
	}
#ifdef _DEBUG
	// 最大速度条
	GetDlgItem(IDC_SLIDER_V_MAX)->ShowWindow(!bShow);
	GetDlgItem(IDC_SLIDER_W_MAX)->ShowWindow(!bShow);
	GetDlgItem(IDC_TEXT_V_MAX)->ShowWindow(!bShow);
	GetDlgItem(IDC_TEXT_W_MAX)->ShowWindow(!bShow);
#endif
}

// 指令模式按钮
void CPM1StarterDlg::OnBnClickedBtnInstruction()
{
	if (m_WorkMode != WorkMode::INSTRUCTION)
	{
		m_btnInst.Select(true);
		m_btnReal.Select(false);
		m_WorkMode = WorkMode::INSTRUCTION;
		ShowInstruction(TRUE);
	}
}

// 实时模式按钮
void CPM1StarterDlg::OnBnClickedBtnRealtime()
{
	if (m_WorkMode != WorkMode::REALTIME)
	{
		m_btnInst.Select(false);
		m_btnReal.Select(true);
		m_WorkMode = WorkMode::REALTIME;
		ShowInstruction(FALSE);
	}
}

// 设置参数界面
void CPM1StarterDlg::SetParamView(CString name1, CString unit1,
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
void CPM1StarterDlg::SetEditText(int index, CString text)
{
	if (index >= 0 && index < 4)
	{
		m_arrAutoModify[index] = TRUE;
		GetDlgItem(IDC_EDIT_V + 10 * index)->SetWindowText(text);
	}
}

// 计算slider与值的比值
double CPM1StarterDlg::CalcSliderK(double min, double max)
{
	double value = max(abs(min), abs(max));
	CString sVal;
	sVal.Format(L"%e", value);
	int first = _ttoi(sVal.Left(1));
	int digit = _ttoi(sVal.Mid(sVal.Find('e') + 1));
	if (first == 1)
	{
		return pow(10, 2 - digit);
	}
	else if (first < 4)
	{
		return 5 * pow(10, 1 - digit);
	}
	else
	{
		return pow(10, 1 - digit);
	}
}

// 界面切换到直行指令状态
void CPM1StarterDlg::SwitchToLine()
{
	// 界面
	SetParamView(L"速度", L"m/s", L"路程", L"m", FALSE);
	// 文本框
	CString sVal;
	// edit
	m_editV.SetRange(-LSPEED_MAX/*LSPEED_MIN*/, LSPEED_MAX);
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
	m_sliderV.SetRange(-LSPEED_MAX/*LSPEED_MIN*/ * m_dLk, LSPEED_MAX * m_dLk);
	m_sliderV.SetTicFreq(1);
	m_sliderV.SetPos(m_arrSpeed[0] * m_dLk);
	// slider
	m_sliderS.SetRange(0, 100);
	m_sliderS.SetTicFreq(1);
	m_sliderS.SetPos(m_arrForward[0] * 10);
	// slider
	m_sliderT.SetRange(0, 60);
	m_sliderT.SetTicFreq(1);
	m_sliderT.SetPos(m_arrTime[0]);
	m_bInputLegal = m_editV.m_bInputOk && m_editS.m_bInputOk && m_editT.m_bInputOk;
	GetDlgItem(IDC_TEXT_T)->EnableWindow(m_arrTimeParam[0]);
	GetDlgItem(IDC_TEXT_S)->EnableWindow(!m_arrTimeParam[0]);
}

// 界面切换到转弯指令状态
void CPM1StarterDlg::SwitchToArc()
{
	// 界面
	SetParamView(L"速度", L"m/s", L"角度", L"degree", TRUE);
	// 文本框
	CString sVal;
	// edit
	m_editV.SetRange(-LSPEED_MAX/*LSPEED_MIN*/, LSPEED_MAX);
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
	m_sliderV.SetRange(-LSPEED_MAX/*LSPEED_MIN*/ * m_dLk, LSPEED_MAX * m_dLk);
	m_sliderV.SetTicFreq(1);
	m_sliderV.SetPos(m_arrSpeed[1] * m_dLk);
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
	m_bInputLegal = m_editV.m_bInputOk && m_editS.m_bInputOk && m_editT.m_bInputOk && m_editR.m_bInputOk;
	GetDlgItem(IDC_TEXT_T)->EnableWindow(m_arrTimeParam[1]);
	GetDlgItem(IDC_TEXT_S)->EnableWindow(!m_arrTimeParam[1]);
}

// 界面切换到原地转弯指令状态
void CPM1StarterDlg::SwitchToAround()
{
	// 界面
	SetParamView(L"角速度", L"degree/s", L"角度", L"degree", FALSE);
	// 文本框
	CString sVal;
	// edit
	m_editV.SetRange(-ASPEED_MAX/*ASPEED_MIN*/, ASPEED_MAX);
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
	m_sliderV.SetRange(-ASPEED_MAX/*ASPEED_MIN*/ * m_dAk, ASPEED_MAX * m_dAk);
	m_sliderV.SetTicFreq(1);
	m_sliderV.SetPos(m_arrSpeed[2] * m_dAk);
	// slider
	m_sliderS.SetRange(0, 360);
	m_sliderS.SetTicFreq(1);
	m_sliderS.SetPos(m_arrForward[2]);
	// slider
	m_sliderT.SetRange(0, 60);
	m_sliderT.SetTicFreq(1);
	m_sliderT.SetPos(m_arrTime[2]);
	m_bInputLegal = m_editV.m_bInputOk && m_editS.m_bInputOk && m_editT.m_bInputOk;
	GetDlgItem(IDC_TEXT_T)->EnableWindow(m_arrTimeParam[2]);
	GetDlgItem(IDC_TEXT_S)->EnableWindow(!m_arrTimeParam[2]);
}

// 指令切换
void CPM1StarterDlg::OnCbnSelchangeCbInstruction()
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
	CString sInst;
	m_cbInstruction.GetWindowText(sInst);
	GetDlgItem(IDC_TEXT_INSTRUCTION)->SetWindowText(sInst);
	GetDlgItem(IDC_BTN_GO)->EnableWindow(m_bInputLegal && m_bConnected);
	CRect rect;
	GetWindowRect(rect);
	rect.SetRect(m_nLineLeft, m_nLineTop, rect.Width(), m_nLineBottom);
	InvalidateRect(rect);
}

// 刷新运动示意图
void CPM1StarterDlg::RefreshBitmap(int select, double speed, double radius)
{
	int index = m_nBitmapIndex;
	if (select == 0)
	{
		if (speed > 0)
		{
			index = 0;	// 前
		}
		else if (speed < 0)
		{
			index = 1;	// 后
		}
		else
		{
			index = 9;	// 停止
		}
	}
	else if (select == 1)
	{
		if (speed > 0 && radius > 0)
		{
			index = 2;	// 左前
		}
		else if (speed > 0 && radius < 0)
		{
			index = 3;	// 右前
		}
		else if (speed < 0 && radius > 0)
		{
			index = 4;	// 左后
		}
		else if (speed < 0 && radius < 0)
		{
			index = 5;	// 右后
		}
		else
		{
			index = 9;	// 停止
		}
	}
	else if (select == 2)
	{
		if (speed > 0)
		{
			index = 6;	// 逆时针
		}
		else if (speed < 0)
		{
			index = 7;	// 顺时针
		}
		else
		{
			index = 9;	// 停止
		}
	}
	if (m_nBitmapIndex != index)
	{
		((CStatic*)GetDlgItem(IDC_PIC_TIP))->SetBitmap(m_hBitmap[index]);
	}
	m_nBitmapIndex = index;
}

// 文本框文本变化响应
void CPM1StarterDlg::OnEnChangeEdit(CMyEdit* pEdit, CSliderCtrl* pSlider, int index)
{
	// 判断指令索引
	int select = m_cbInstruction.GetCurSel();
	if (select < 0 || select >= INST_NUM)
	{
		return;
	}
	// 读取编辑框值
	CString text;
	pEdit->GetWindowText(text);
	double value = pEdit->m_bInputOk ? _ttof(text) : 0;
	// 更新参数值
	((double*)pEdit->m_Tag)[pEdit == &m_editR ? 0 : select] = value;
	// 更新运动示意图
	if (pEdit == &m_editV || pEdit == &m_editR)
	{
		RefreshBitmap(select, m_arrSpeed[select], m_dRadius);
	}
	// 设置滑块
	int pos = int(value);
	if (pEdit == &m_editV)
	{
		pos = int(select == 2 ? value * m_dAk : value * m_dLk);
	}
	else if ((pEdit == &m_editS && select == 0) || pEdit == &m_editR)
	{
		pos = int(value * 10);
	}
	pSlider->SetPos(pos);
	// 非手动修改则返回
	if (m_arrAutoModify[index])
	{
		m_arrAutoModify[index] = FALSE;
		return;
	}
	// 自动更新其他参数值
	double v_rad_deg = (select == 1) ? (PI * m_dRadius / 180) : 1;
	if (pEdit == &m_editT)
	{
		m_arrForward[select] = fabs(m_arrSpeed[select] * m_arrTime[select] / v_rad_deg);
		// 更新路程参数显示
		m_editS.GetWindowText(text);
		value = _ttof(text);
		if (fabs(value - m_arrForward[select]) > EPSILON)
		{
			text.Format(L"%.2f", m_arrForward[select]);
			SetEditText(1, text.TrimRight('0').TrimRight('.'));
		}
		// 选中时间参数API
		m_arrTimeParam[select] = true;
		GetDlgItem(IDC_TEXT_T)->EnableWindow(TRUE);
		GetDlgItem(IDC_TEXT_S)->EnableWindow(FALSE);
	}
	else if (m_arrSpeed[select] != 0)
	{
		m_arrTime[select] = fabs(m_arrForward[select] / m_arrSpeed[select] * v_rad_deg);
		// 更新时间参数显示
		m_editT.GetWindowText(text);
		value = _ttof(text);
		if (fabs(value - m_arrTime[select]) > EPSILON)
		{
			text.Format(L"%.2f", m_arrTime[select]);
			SetEditText(2, text.TrimRight('0').TrimRight('.'));
		}
		// 选中路程参数API
		if (pEdit == &m_editS)
		{
			m_arrTimeParam[select] = false;
			GetDlgItem(IDC_TEXT_T)->EnableWindow(FALSE);
			GetDlgItem(IDC_TEXT_S)->EnableWindow(TRUE);
		}
	}
	// 参数合法性
	m_bInputLegal = m_editV.m_bInputOk && m_editS.m_bInputOk && m_editT.m_bInputOk && 
		(select != 1 || m_editR.m_bInputOk);
	GetDlgItem(IDC_BTN_GO)->EnableWindow(m_bInputLegal && m_bConnected);
}

// 文本框文本变化响应
void CPM1StarterDlg::OnEnChangeEditV()
{
	OnEnChangeEdit(&m_editV, &m_sliderV, 0);
}

// 文本框文本变化响应
void CPM1StarterDlg::OnEnChangeEditS()
{
	OnEnChangeEdit(&m_editS, &m_sliderS, 1);
}

// 文本框文本变化响应
void CPM1StarterDlg::OnEnChangeEditT()
{
	OnEnChangeEdit(&m_editT, &m_sliderT, 2);
}

// 文本框文本变化响应
void CPM1StarterDlg::OnEnChangeEditR()
{
	OnEnChangeEdit(&m_editR, &m_sliderR, 3);
}

// 水平滚动条响应事件
void CPM1StarterDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CString text;
	int select = m_cbInstruction.GetCurSel(); // 指令索引
	if (&m_sliderV == (CSliderCtrl*)pScrollBar)
	{
		double pos = m_sliderV.GetPos(); //SB_THUMBTRACK
		text.Format(L"%.2f", select == 2 ? pos / m_dAk : pos / m_dLk);
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

// 垂直滚动条响应事件
void CPM1StarterDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int pos = ((CSliderCtrl*)pScrollBar)->GetPos();
	if (GetDlgItem(IDC_SLIDER_V_MAX) == pScrollBar)
	{
		double v = (20 - pos) * 2.0 / 20.0;
		LSPEED_MAX = v;
		LSPEED_MAX_JOY = v;
		CString text;
		text.Format(L"%.1fm/s", v);
		GetDlgItem(IDC_TEXT_V_MAX)->SetWindowText(text);
	}
	else if (GetDlgItem(IDC_SLIDER_W_MAX) == pScrollBar)
	{
		int w = 50 - pos;
		ASPEED_MAX = w;
		ASPEED_MAX_JOY = w;
		CString text;
		text.Format(L"%d°/s", w);
		GetDlgItem(IDC_TEXT_W_MAX)->SetWindowText(text);
	}

	CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
}

// 点击运动示意图切换方向(单击)
void CPM1StarterDlg::OnStnClickedPicTip()
{
	if (!m_bExecuting)
	{
		CString text;
		int select = m_cbInstruction.GetCurSel();// 指令索引
		if (m_nBitmapIndex == 0 || m_nBitmapIndex == 1 ||	// 前/后
			m_nBitmapIndex == 3 || m_nBitmapIndex == 4 ||	// 右前/左后
			m_nBitmapIndex == 6 || m_nBitmapIndex == 7)		// 逆时针/顺时针
		{
			text.Format(L"%.2f", -m_arrSpeed[select]);
			SetEditText(0, text.TrimRight('0').TrimRight('.'));
		}
		else if (m_nBitmapIndex == 2 || m_nBitmapIndex == 5)// 左前/右后
		{
			text.Format(L"%.2f", -m_dRadius);
			SetEditText(3, text.TrimRight('0').TrimRight('.'));
		}
	}
}

// 点击运动示意图切换方向(双击)
void CPM1StarterDlg::OnStnDblclickPicTip()
{
	OnStnClickedPicTip();
}

// 进度条线程
DWORD WINAPI ProgressThread(LPVOID lpParm)
{
	CPM1StarterDlg* pDlg = (CPM1StarterDlg*)lpParm;
	int lower, upper;
	pDlg->m_bProgress = true;
	pDlg->m_bAbort = false;
	pDlg->m_progressGo.GetRange(lower, upper);
	while (pDlg->m_bProgress)
	{
		delay(0.05);
		pDlg->m_progressGo.SetPos((int)((upper - lower) * pDlg->m_dProgress + lower));
		
	}
	if (!pDlg->m_bAbort)
	{
		delay(0.3);
	}
	pDlg->m_bAbort = false;
	pDlg->m_progressGo.SetPos(0);
	return 0;
}

// 指令执行线程
DWORD WINAPI InstructionThread(LPVOID lpParm)
{
	CPM1StarterDlg* pDlg = (CPM1StarterDlg*)lpParm;
	pDlg->m_bExecuting = true;	// 进入指令执行状态
	pDlg->GetDlgItem(IDC_TEXT_JOY_INFO)->SetWindowText(sJoyTip2);	// 设置摇杆提示文字
	pDlg->m_Tip.DelTool(pDlg->GetDlgItem(IDC_PIC_TIP));
	std::thread thread(ProgressThread, lpParm);		// 启动进度条
	int index = pDlg->m_cbInstruction.GetCurSel();	// 指令索引
	switch (index)
	{
	case 0:	// 直行
		resume();
		if (!pDlg->m_arrTimeParam[0])
		{
			go_straight(pDlg->m_arrSpeed[0], pDlg->m_arrForward[0], &pDlg->m_dProgress);
		}
		else
		{
			go_straight_timing(pDlg->m_arrSpeed[0], pDlg->m_arrTime[0], &pDlg->m_dProgress);
		}
#ifdef _DEBUG
		if (debug)
		{
			double t = pDlg->m_arrTime[0] / 10;
			for (int i = 0; i < 10; i++)
			{
				delay(t);
				pDlg->m_dProgress = (i + 1) / 10.0;
				if (pDlg->m_bAbort)
				{
					break;
				}
			}
		}
#endif
		break;
	case 1:	// 转弯
		resume();
		if (!pDlg->m_arrTimeParam[1])
		{
			go_arc_va(pDlg->m_arrSpeed[1], pDlg->m_dRadius, pDlg->m_arrForward[1] * PI / 180, &pDlg->m_dProgress);
		}
		else
		{
			go_arc_vt(pDlg->m_arrSpeed[1], pDlg->m_dRadius, pDlg->m_arrTime[1], &pDlg->m_dProgress);
		}
		break;
	case 2:	// 原地转
		resume();
		if (!pDlg->m_arrTimeParam[2])
		{
			turn_around(pDlg->m_arrSpeed[2] * PI / 180, pDlg->m_arrForward[2] * PI / 180, &pDlg->m_dProgress);
		}
		else
		{
			turn_around_timing(pDlg->m_arrSpeed[2] * PI / 180, pDlg->m_arrTime[2], &pDlg->m_dProgress);
		}
		break;
	default:
		break;
	}
	pDlg->m_bProgress = false;
	thread.join();	// 等待进度条结束
	// 更新指令显示
	pDlg->GetDlgItem(IDC_BTN_GO)->SetWindowText(L"执行");
	pDlg->m_cbInstruction.EnableWindow(TRUE);
	for (int i = IDC_TEXT_V; i <= IDC_TEXT_R_U; i++)
	{
		if (pDlg->GetDlgItem(i))
		{
			pDlg->GetDlgItem(i)->EnableWindow(TRUE);
		}
	}
	pDlg->GetDlgItem(IDC_TEXT_T)->EnableWindow(pDlg->m_arrTimeParam[index]);
	pDlg->GetDlgItem(IDC_TEXT_S)->EnableWindow(!pDlg->m_arrTimeParam[index]);
	((CStatic*)pDlg->GetDlgItem(IDC_PIC_TIP))->SetBitmap(pDlg->m_hBitmap[pDlg->m_nBitmapIndex]);
	pDlg->m_Tip.AddTool(pDlg->GetDlgItem(IDC_PIC_TIP), L"点击切换运动方向");
	// 离开指令执行状态
	pDlg->m_bExecuting = false;
	// 更新摇杆显示
	pDlg->GetDlgItem(IDC_TEXT_JOY_INFO)->SetWindowText(sJoyTip1);// 设置摇杆提示文字
	if (pDlg->m_WorkMode == WorkMode::REALTIME)
	{
		pDlg->InvalidateRect(pDlg->m_JoystickRect, FALSE);
	}
	return 0;
}

// 执行/暂停/恢复
void CPM1StarterDlg::OnBnClickedBtnGo()
{
	CString sName;
	GetDlgItem(IDC_BTN_GO)->GetWindowText(sName);
	int index = m_cbInstruction.GetCurSel(); // 指令索引
	if (sName == L"执行")
	{
		// 启线程执行指令
		m_hInstructionThread = CreateThread(NULL, 0, InstructionThread, this, 0, NULL);
		// 定时更新进度条
		GetDlgItem(IDC_BTN_GO)->SetWindowText(L"暂停");
		m_cbInstruction.EnableWindow(FALSE);
		for (int i = IDC_TEXT_V; i <= IDC_TEXT_R_U; i++)
		{
			if ((i % 10 == 1 || i % 10 == 2) && GetDlgItem(i))
			{
				GetDlgItem(i)->EnableWindow(FALSE);
			}
		}
	}
	else if (sName == L"暂停")
	{
		GetDlgItem(IDC_BTN_GO)->SetWindowText(L"恢复");
		pause();
		((CStatic*)GetDlgItem(IDC_PIC_TIP))->SetBitmap(m_hBitmap[8]);
	}
	else
	{
		GetDlgItem(IDC_BTN_GO)->SetWindowText(L"暂停");
		resume();
		((CStatic*)GetDlgItem(IDC_PIC_TIP))->SetBitmap(m_hBitmap[m_nBitmapIndex]);
	}
}

// 终止按钮
void CPM1StarterDlg::OnBnClickedBtnAbort()
{
	if (m_bConnected)
	{
		m_bAbort = true;
		pause();		// 先刹车
		cancel_action();// 再取消指令
	}
}

// 实时控制线程
DWORD WINAPI DriveThread(LPVOID lpParm)
{
	CPM1StarterDlg* pDlg = (CPM1StarterDlg*)lpParm;
	while (pDlg->m_bJoystickDown || pDlg->m_bDirectionKey)
	{
		if (pDlg->m_dLSpeed >= 0)
		{
			drive(pDlg->m_dLSpeed, pDlg->m_dASpeed * PI / 180);
		}
		else // 后退时速度减半
		{
			drive(pDlg->m_dLSpeed / 2, pDlg->m_dASpeed * PI / 180 / 2);
		}
		//TRACE(L"\n=====%f,%f\n", pDlg->m_dLSpeed, pDlg->m_dASpeed * PI / 180);
		delay(0.05);
	}
	drive(0, 0);
	return 0;
}

// 启动实时控制线程
void CPM1StarterDlg::StartDrive()
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
void CPM1StarterDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 摇杆
	if (m_WorkMode == WorkMode::REALTIME && !m_bExecuting && !KillTimer(2))
	{
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
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

// 虚拟摇杆-鼠标左键抬起 
void CPM1StarterDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bJoystickDown)
	{
		SetTimer(2, 150, NULL);
	}

	CDialogEx::OnLButtonUp(nFlags, point);
}

// 虚拟摇杆-鼠标移动
void CPM1StarterDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// 摇杆
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
		// 根据摇杆位置设置速度
		m_dLSpeed = -2.0 * (point.y - m_JoystickRect.CenterPoint().y) / 
			m_JoystickRect.Height() * LSPEED_MAX_JOY;
		m_dASpeed = -2.0 * (point.x - m_JoystickRect.CenterPoint().x) / 
			m_JoystickRect.Width() * ASPEED_MAX_JOY;
		// 设置原地转向区域
		int dx = abs(point.x - x);
		int dy = abs(point.y - y);
		if (dy < dx * ROTATION_TAN)
		{
			m_dLSpeed = 0;
		}
		// 调整倒车转弯方向
		else
		{
			m_dASpeed *= m_dLSpeed > 0 ? 1 : -1;
		}
		// 刷新显示
		int r = m_JoystickRect.Width() / 10;
		CRect rect(m_JoystickRect.left - r, m_JoystickRect.top - r,
			m_JoystickRect.right + r, m_JoystickRect.bottom + r);
		InvalidateRect(rect, FALSE);
		SetCursor(LoadCursor(NULL, IDC_SIZEALL));
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

// PreTranslateMessage
BOOL CPM1StarterDlg::PreTranslateMessage(MSG* pMsg)
{
	UINT nKey = (UINT)pMsg->wParam;

	// 快捷按键
	if (WM_KEYDOWN == pMsg->message)
	{
		if (VK_RETURN == nKey)		// 屏蔽Enter
		{
			return TRUE;
		}
		else if (VK_ESCAPE == nKey)	// Esc终止指令
		{
			OnBnClickedBtnAbort();
			return TRUE;
		}
		else if (VK_SPACE == nKey)	// 空格暂停/恢复指令
		{
			if (m_WorkMode == WorkMode::INSTRUCTION && m_bExecuting)
			{
				OnBnClickedBtnGo();
			}
			return TRUE;
		}
	}

	// 方向键
	if ((WM_KEYDOWN == pMsg->message || WM_KEYUP == pMsg->message) &&	// 按下抬起
		(VK_UP == nKey || VK_DOWN == nKey || VK_LEFT == nKey || VK_RIGHT == nKey) && // 方向键
		m_WorkMode == WorkMode::REALTIME &&		// 连续控制模式
		!m_bJoystickDown &&						// 未操作摇杆
		!m_bExecuting)							// 未执行指令
	{
		bool keydown = WM_KEYDOWN == pMsg->message;
		// 更新按键状态
		switch (nKey)
		{
		case VK_UP:		// 上
			m_bUp = keydown;
			break;
		case VK_DOWN:	// 下
			m_bDown = keydown;
			break;
		case VK_LEFT:	// 左
			m_bLeft = keydown;
			break;
		case VK_RIGHT:	// 右
			m_bRight = keydown;
			break;
		default:
			break;
		}
		// 启动连续控制线程
		if (keydown && !m_bDirectionKey)
		{
			m_bDirectionKey = true;
			StartDrive();
		}
		// 运动参数
		// 速度
		if (m_bUp && !m_bDown)
		{
			m_dLSpeed = LSPEED_MAX_JOY / 2;
			m_JoystickPos.y = m_JoystickRect.top + m_JoystickRect.Height() / 4;
		}
		else if (m_bDown && !m_bUp)
		{
			m_dLSpeed = -LSPEED_MAX_JOY / 2;
			m_JoystickPos.y = m_JoystickRect.top + m_JoystickRect.Height() * 3 / 4;
		}
		else
		{
			m_dLSpeed = 0;
			m_JoystickPos.y = m_JoystickRect.CenterPoint().y;
		}
		// 转向
		if (m_bLeft && !m_bRight)
		{
			m_dASpeed = ASPEED_MAX_JOY / (m_dLSpeed >= 0 ? 3 : -3);
			m_JoystickPos.x = m_JoystickRect.left + m_JoystickRect.Width() / 4;
		}
		else if (m_bRight && !m_bLeft)
		{
			m_dASpeed = -ASPEED_MAX_JOY / (m_dLSpeed >= 0 ? 3 : -3);
			m_JoystickPos.x = m_JoystickRect.left + m_JoystickRect.Width() * 3 / 4;
		}
		else
		{
			m_dASpeed = 0;
			m_JoystickPos.x = m_JoystickRect.CenterPoint().x;
		}
		m_bDirectionKey = m_bUp || m_bDown || m_bLeft || m_bRight;
		InvalidateRect(m_JoystickRect, FALSE);
		return TRUE;
	}

	// 提示信息
	if (WM_MOUSEMOVE == pMsg->message)
	{
		m_Tip.RelayEvent(pMsg);
	}

	// 状态图片事件处理
	if (pMsg->hwnd == GetDlgItem(IDC_PIC_STATE)->m_hWnd)
	{
		switch (pMsg->message)
		{
		case WM_MOUSEHOVER:
			m_bStateHover = true;
			RefreshState();
			break;
		case WM_MOUSELEAVE:
			m_bStateHover = false;
			RefreshState();
			break;
		case WM_MOUSEMOVE:
			if (!m_bStateHover)
			{
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.hwndTrack = pMsg->hwnd;
				tme.dwFlags = TME_LEAVE | TME_HOVER;
				tme.dwHoverTime = 1;
				m_bStateHover = _TrackMouseEvent(&tme); // post进入离开事件
			}
			break;
		default:
			break;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

// 定时器(取里程计数据/锁定状态数据/失焦点处理)
void CPM1StarterDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
		case 1:
		{
			// 里程计数据
			odometry odom = get_odometry().value;
			if (!isnan(odom.x) && !isnan(odom.y) && !isnan(odom.yaw))
			{
				CString text;
				double x = abs(odom.x) < 0.005 ? 0 : odom.x;
				double y = abs(odom.y) < 0.005 ? 0 : odom.y;
				double yaw = abs(odom.yaw * 180 / PI) < 0.005 ? 0 : (odom.yaw * 180 / PI);
				text.Format(L"%.2f m\n%.2f m\n%.2f °", x, y, yaw);
				GetDlgItem(IDC_TEXT_ODOM)->SetWindowText(text);
			}
			// 锁定状态数据
			switch (check_state())
			{
			case chassis_state::unlocked:
				SetState(PM1State::UNLOCK);
				break;
			case chassis_state::locked:
				SetState(PM1State::LOCK);
				break;
			case chassis_state::offline:
				Disconnect();
				SetState(PM1State::UNCONNECT);
				break;
			default:
				SetState(PM1State::UNKNOW);
				break;
			}
			// 连续控制过程中窗口失去焦点则自动终止所有控制动作
			if (m_WorkMode == WorkMode::REALTIME)	// 连续控制模式
			{
				static bool g_top = true;
				bool top = this->GetSafeHwnd() == ::GetForegroundWindow();
				// 窗口不再最前
				if (g_top && !top)
				{
					// 模拟抬起方向键
					if (m_bUp)
					{
						PostMessage(WM_KEYUP, VK_UP, 0);
					}
					if (m_bDown)
					{
						PostMessage(WM_KEYUP, VK_DOWN, 0);
					}
					if (m_bLeft)
					{
						PostMessage(WM_KEYUP, VK_LEFT, 0);
					}
					if (m_bRight)
					{
						PostMessage(WM_KEYUP, VK_RIGHT, 0);
					}
					// 模拟抬起鼠标左键
					if (m_bJoystickDown)
					{
						PostMessage(WM_LBUTTONUP);
					}
				}
				g_top = top;
			}
			
		}
		break;
		case 2:
		{
			// 摇杆抬起
			m_bJoystickDown = false;
			m_JoystickPos.SetPoint(m_JoystickRect.CenterPoint().x, m_JoystickRect.CenterPoint().y);
			m_dLSpeed = 0;
			m_dASpeed = 0;
			int r = m_JoystickRect.Width() / 10;
			CRect rect(m_JoystickRect.left - r, m_JoystickRect.top - r,
				m_JoystickRect.right + r, m_JoystickRect.bottom + r);
			InvalidateRect(rect, FALSE);
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			ReleaseCapture();
			KillTimer(2);
		}
		break;
	default:
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

// 设置状态
void CPM1StarterDlg::SetState(PM1State state)
{
	if (m_state != state)
	{
		m_state = state;
		((CStatic*)GetDlgItem(IDC_PIC_STATE))->SetBitmap(m_hBitmap[10 + m_state + 
			((m_state == PM1State::LOCK || m_state == PM1State::UNLOCK) && m_bStateHover ? 3 : 0)]);
		m_Tip.DelTool(GetDlgItem(IDC_PIC_STATE));
		switch (state)
		{
		case PM1State::UNCONNECT:
			m_Tip.AddTool(GetDlgItem(IDC_PIC_STATE), L"未连接");
			break;
		case PM1State::CONNECT:
			m_Tip.AddTool(GetDlgItem(IDC_PIC_STATE), L"已连接");
			break;
		case PM1State::LOCK:
			m_Tip.AddTool(GetDlgItem(IDC_PIC_STATE), L"PM1已锁定,点击解锁");
			break;
		case PM1State::UNLOCK:
			m_Tip.AddTool(GetDlgItem(IDC_PIC_STATE), L"PM1已解锁,点击锁定");
			break;
		case PM1State::UNKNOW:
			m_Tip.AddTool(GetDlgItem(IDC_PIC_STATE), L"错误状态");
			break;
		default:
			break;
		}
	}
}

// 刷新状态显示
void CPM1StarterDlg::RefreshState()
{
	if (m_state == PM1State::LOCK || m_state == PM1State::UNLOCK)
	{
		((CStatic*)GetDlgItem(IDC_PIC_STATE))->SetBitmap(m_hBitmap[10 + m_state + (m_bStateHover ? 3 : 0)]);
	}
}

// 里程计"清零"
void CPM1StarterDlg::OnNMClickSyslinkClear(NMHDR *pNMHDR, LRESULT *pResult)
{
	reset_odometry();
	*pResult = 0;
}

// 解锁/锁定
void CPM1StarterDlg::OnStnClickedPicState()
{
	if (m_state == PM1State::LOCK)
	{
		unlock();
	}
	else if (m_state == PM1State::UNLOCK)
	{
		lock();
	}
}

// 改变窗口大小时重新布局
void CPM1StarterDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (!m_bInit)
	{
		return;
	}
	int width = cx;
	int height = cy;
	// 关闭按钮
	CRect rect1;
	m_btnClose.GetWindowRect(rect1);
	m_btnClose.MoveWindow(width - rect1.Width() - 1, 1, rect1.Width(), rect1.Height());
	// 最小化按钮
	CRect rect2;
	m_btnMinimize.LoadBitmaps(IDB_MINIMIZE_NORM, IDB_MINIMIZE_OVER, IDB_MINIMIZE_HIT);
	m_btnMinimize.GetWindowRect(rect2);
	m_btnMinimize.MoveWindow(width - rect1.Width() - rect2.Width() - 1, 1,
		rect2.Width(), rect2.Height());
	// 关于按钮
	CRect rect3;
	m_btnAbout.LoadBitmaps(IDB_ABOUT_NORM, IDB_ABOUT_OVER, IDB_ABOUT_HIT);
	m_btnAbout.GetWindowRect(rect3);
	m_btnAbout.MoveWindow(width - rect1.Width() - rect2.Width() - rect3.Width() - 1, 1,
		rect3.Width(), rect3.Height());
	// 状态图标
	CRect rect;
	GetDlgItem(IDC_PIC_STATE)->GetWindowRect(rect);
	GetDlgItem(IDC_PIC_STATE)->MoveWindow(width - rect.Width() - 15,
		m_nLineTop - rect.Height() - 15, rect.Width(), rect.Height());
	// 里程计行标题
	GetDlgItem(IDC_STATIC_ODOM_TITLE)->GetWindowRect(rect);
	GetDlgItem(IDC_STATIC_ODOM_TITLE)->MoveWindow(10, height - rect.Height() - 5, rect.Width(), rect.Height());
	GetDlgItem(IDC_STATIC_ODOM_TITLE)->GetWindowRect(rect);
	ScreenToClient(rect);
	// 里程计值
	GetDlgItem(IDC_TEXT_ODOM)->GetWindowRect(rect1);
	GetDlgItem(IDC_TEXT_ODOM)->MoveWindow(rect.right, rect.top, m_nLineLeft - rect.right, rect1.Height());
	// "里程计"
	GetDlgItem(IDC_STATIC_ODOM)->GetWindowRect(rect1);
	GetDlgItem(IDC_STATIC_ODOM)->MoveWindow(rect.left, rect.top - rect1.Height() - 10, rect1.Width(), rect1.Height());
	GetDlgItem(IDC_STATIC_ODOM)->GetWindowRect(rect);
	ScreenToClient(rect);
	// "清零"
	GetDlgItem(IDC_SYSLINK_CLEAR)->GetWindowRect(rect1);
	GetDlgItem(IDC_SYSLINK_CLEAR)->MoveWindow(rect.Width() + 25, rect.top, rect1.Width(), rect1.Height());
	// 底内线
	GetDlgItem(IDC_BTN_ABORT)->GetWindowRect(rect);
	m_nLineBottom = height - rect.Height() - 20;
	// 参数
	GetDlgItem(IDC_TEXT_V)->GetWindowRect(rect);
	ScreenToClient(rect);
	int unit_height = (m_nLineBottom - rect.top) / 5;
	GetDlgItem(IDC_TEXT_V_U)->GetWindowRect(rect1);
	ScreenToClient(rect1);
	GetDlgItem(IDC_EDIT_V)->GetWindowRect(rect2);
	ScreenToClient(rect2);
	GetDlgItem(IDC_SLIDER_V)->GetWindowRect(rect3);
	ScreenToClient(rect3);
	for (int i = 0; i < 4; i++)
	{
		GetDlgItem(IDC_TEXT_V + i * 10)->MoveWindow(rect.left, rect.top + unit_height * i, rect.Width(), rect.Height());
		GetDlgItem(IDC_TEXT_V_U + i * 10)->MoveWindow(width - 80, rect1.top + unit_height * i, rect1.Width(), rect1.Height());
		GetDlgItem(IDC_EDIT_V + i * 10)->MoveWindow(width - 80 - 55, rect2.top + unit_height * i, rect2.Width(), rect2.Height());
		GetDlgItem(IDC_SLIDER_V + i * 10)->MoveWindow(rect.left + 45, rect3.top + unit_height * i,
			width - rect.left - 45 - 5 - 55 - 80, rect3.Height());
	}
	// 执行按钮
	GetDlgItem(IDC_BTN_GO)->GetWindowRect(rect);
	GetDlgItem(IDC_BTN_GO)->MoveWindow(width - 55 - 80, 
		rect2.top + unit_height * (m_cbInstruction.GetCurSel() == 1 ? 4 : 3), rect.Width(), rect.Height());
	// 进度条左侧命令指示
	GetDlgItem(IDC_TEXT_INSTRUCTION)->GetWindowRect(rect1);
	ScreenToClient(rect1);
	GetDlgItem(IDC_TEXT_INSTRUCTION)->MoveWindow(rect1.left, m_nLineBottom + 14, rect1.Width(), rect1.Height());
	// 终止按钮
	GetDlgItem(IDC_BTN_ABORT)->GetWindowRect(rect);
	GetDlgItem(IDC_BTN_ABORT)->MoveWindow(width - 82, m_nLineBottom + 10, rect.Width(), rect.Height());
	// 进度条
	GetDlgItem(IDC_PROGRESS_GO)->GetWindowRect(rect);
	GetDlgItem(IDC_PROGRESS_GO)->MoveWindow(rect1.right, m_nLineBottom + 16, width - 82 - 20 - rect1.right, rect.Height());
	// 摇杆提示
	GetDlgItem(IDC_TEXT_JOY_INFO)->GetWindowRect(rect);
	ScreenToClient(rect);
	GetDlgItem(IDC_TEXT_JOY_INFO)->MoveWindow(m_nLineLeft + 1, m_nLineTop + 5,
		width - m_nLineLeft - 2, rect.Height());
	// 摇杆尺寸位置
	int d = (m_nLineBottom - rect.bottom) / 13;
	int left = (width - m_nLineLeft - 10 * d) / 2 + m_nLineLeft;
	int top = rect.bottom + 1.5 * d;
	m_JoystickRect.SetRect(left, top, left + 10 * d, top + 10 * d);
	m_JoystickPos.SetPoint(m_JoystickRect.CenterPoint().x, m_JoystickRect.CenterPoint().y);
#ifdef _DEBUG
	// 最大速度
	GetDlgItem(IDC_STATIC_ODOM)->GetWindowRect(rect1);
	ScreenToClient(rect1);
	GetDlgItem(IDC_SLIDER_V_MAX)->GetWindowRect(rect);
	ScreenToClient(rect);
	GetDlgItem(IDC_TEXT_V_MAX)->GetWindowRect(rect2);
	GetDlgItem(IDC_SLIDER_V_MAX)->MoveWindow(rect.left, rect.top, rect.Width(), rect1.top - 22 - rect.top - rect2.Height());
	GetDlgItem(IDC_SLIDER_V_MAX)->GetWindowRect(rect);
	ScreenToClient(rect);
	GetDlgItem(IDC_TEXT_V_MAX)->MoveWindow(rect.left - 4, rect.bottom + 2, rect2.Width(), rect2.Height());
	GetDlgItem(IDC_SLIDER_W_MAX)->GetWindowRect(rect);
	ScreenToClient(rect);
	GetDlgItem(IDC_TEXT_W_MAX)->GetWindowRect(rect2);
	GetDlgItem(IDC_SLIDER_W_MAX)->MoveWindow(rect.left, rect.top, rect.Width(), rect1.top - 22 - rect.top - rect2.Height());
	GetDlgItem(IDC_SLIDER_W_MAX)->GetWindowRect(rect);
	ScreenToClient(rect);
	GetDlgItem(IDC_TEXT_W_MAX)->MoveWindow(rect.left - 2, rect.bottom + 2, rect2.Width(), rect2.Height());
#endif

	Invalidate();
}

// 限制窗口尺寸
void CPM1StarterDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = 589;
	lpMMI->ptMinTrackSize.y = 439;

	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

// 触控响应(解决触摸时不能及时触发按下事件的问题(移动或抬起时才触发))
BOOL CPM1StarterDlg::OnTouchInput(
	CPoint pt, int nInputNumber, int nInputsCount, PTOUCHINPUT pInput)
{
	return FALSE;
}

