// AboutDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "PM1SDKDemo.h"
#include "AboutDlg.h"
#include "afxdialogex.h"

// CAboutDlg 对话框

IMPLEMENT_DYNAMIC(CAboutDlg, CDialogEx)

CAboutDlg::CAboutDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ABOUTBOX, pParent)
{
}

CAboutDlg::~CAboutDlg()
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_CLOSE, m_btnClose);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_WM_NCCALCSIZE()
	ON_WM_NCHITTEST()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BTN_CLOSE, &CAboutDlg::OnBnClickedBtnClose)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

// 初始化
BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CRect winRect;
	GetWindowRect(winRect);
	CRect rect;
	m_btnClose.LoadBitmaps(IDB_CLOSE_NORM, IDB_CLOSE_OVER, IDB_CLOSE_HIT);
	m_btnClose.GetWindowRect(rect);
	m_btnClose.MoveWindow(winRect.Width() - rect.Width() - 1, 1, rect.Width(), rect.Height());
	return TRUE;
}

// 界面绘图
void CAboutDlg::OnPaint()
{
	CPaintDC dc(this);
	CRect rect;
	GetClientRect(rect);
	// 填充标题栏
	CPen titlePen(PS_SOLID, 1, RGB(255, 255, 255));
	CBrush titleBrush(RGB(255, 255, 255));
	CPen* pOldPen = dc.SelectObject(&titlePen);
	CBrush* pOldBrush = dc.SelectObject(&titleBrush);
	dc.Rectangle(0, 0, rect.Width(), 30);
	dc.SelectObject(pOldPen);
	dc.SelectObject(pOldBrush);
	titlePen.DeleteObject();
	titleBrush.DeleteObject();
	// 画窗口边框
	CPen edgePen(PS_SOLID, 1, RGB(112, 112, 112));
	dc.SelectObject(&edgePen);
	dc.MoveTo(0, 0);
	dc.LineTo(rect.Width() - 1, 0);
	dc.LineTo(rect.Width() - 1, rect.Height() - 1);
	dc.LineTo(0, rect.Height() - 1);
	dc.LineTo(0, 0);
	dc.SelectObject(pOldPen);
	edgePen.DeleteObject();
}

// 拦截WM_NCCALCSIZE
void CAboutDlg::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	// 拦截WM_NCCALCSIZE实现无标题栏及边框(保留动画效果)
	//CDialogEx::OnNcCalcSize(bCalcValidRects, lpncsp);
}

// 窗口拖动
LRESULT CAboutDlg::OnNcHitTest(CPoint point)
{
	POINT pt;
	pt.x = point.x;
	pt.y = point.y;
	ScreenToClient(&pt);
	if (pt.y <= 30)
	{
		return HTCAPTION;
	}

	return CDialogEx::OnNcHitTest(point);
}

// 关闭按钮
void CAboutDlg::OnBnClickedBtnClose()
{
	OnCancel();
}

// 颜色设置
HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (CTLCOLOR_STATIC == nCtlColor &&
		IDC_STATIC_ABOUT == pWnd->GetDlgCtrlID())
	{
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}
