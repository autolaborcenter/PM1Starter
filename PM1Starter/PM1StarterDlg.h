
// PM1StarterDlg.h: 头文件
//

#pragma once
#include "MyButton.h"
#include "MyEdit.h"
#include "MyProgress.h"

// 工作模式
enum WorkMode
{
	INSTRUCTION,	// 指令模式
	REALTIME,		// 实时模式
	CALIBRATION		// 校准模式
};

// PM1状态
enum PM1State
{
	UNCONNECT = 0,	// 未连接
	CONNECT,		// 连接
	LOCK,			// 锁定
	UNLOCK,			// 未锁定
	UNKNOW			// 错误
};

// CPM1StarterDlg 对话框
class CPM1StarterDlg : public CDialogEx
{
// 构造
public:
	CPM1StarterDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PM1STARTER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnTouchInput(
		CPoint pt, int nInputNumber, int nInputsCount, PTOUCHINPUT pInput);

public:
	// 提示
	CToolTipCtrl m_Tip;
	// 关闭按钮
	CMyButton m_btnClose;
	// 最小化按钮
	CMyButton m_btnMinimize;
	// 关于按钮
	CMyButton m_btnAbout;
	// 串口名
	CComboBox m_cbCom;
	// 指令类型
	CComboBox m_cbInstruction;
	// 执行进度
	CMyProgress m_progressGo;
	// 速度/角速度slider
	CSliderCtrl m_sliderV;
	// 路程/角度slider
	CSliderCtrl m_sliderS;
	// 时间slider
	CSliderCtrl m_sliderT;
	// 半径slider
	CSliderCtrl m_sliderR;
	// 速度/角速度值
	CMyEdit m_editV;
	// 路程/角度值
	CMyEdit m_editS;
	// 时间值
	CMyEdit m_editT;
	// 半径值
	CMyEdit m_editR;
	// 指令模式按钮
	CMyButton m_btnInst;
	// 实时模式按钮
	CMyButton m_btnReal;

public:
	// 标题栏高度
	int m_nTitleHeight = 50;
	// 标题栏颜色
	COLORREF m_TitleColor = RGB(255, 255, 255);
	// 界面内线条位置
	int m_nLineTop = 100;
	int m_nLineBottom = 354;
	int m_nLineLeft = 105;
	// 窗体背景色
	HBRUSH m_hBkBrush;
	// 窗体左侧选项栏背景色
	HBRUSH m_hTabBkBrush;

	// 初始化完成
	bool m_bInit = false;

	// 运动示意图+状态图
	const static int BMP_NUM = 17;
	HBITMAP m_hBitmap[BMP_NUM];
	int m_nBitmapIndex = 0;

	// 工作模式
	WorkMode m_WorkMode = WorkMode::INSTRUCTION;

	// 摇杆参数
	CRect m_JoystickRect;
	CPoint m_JoystickPos;
	bool m_bJoystickDown;

	// 参数范围
	const static int INST_NUM = 3;// 指令数
	double LSPEED_MAX = 0.3;
	double ASPEED_MAX = 45;
	double LSPEED_MAX_JOY = 0.3;
	double ASPEED_MAX_JOY = 45;
	const double COMMOM_MIN = 0;
	const double COMMON_MAX = 1e5;
	// 速度参数
	double m_arrSpeed[INST_NUM];	// 速度/角速度
	double m_arrForward[INST_NUM];	// 距离/角度
	double m_arrTime[INST_NUM];		// 时间
	double m_dRadius;				// 转弯半径
	// 速度slider与值比例
	double m_dLk;	// 线速度比例
	double m_dAk;	// 角速度比例

	// PM1状态
	PM1State m_state = PM1State::UNCONNECT;
	// 状态图片hover标志
	bool m_bStateHover = false;

	// 连接线程句柄
	HANDLE m_hConnectThread = NULL;

	// 连接状态
	bool m_bConnected = false;

	// 连接提示状态
	bool m_bTipWarn = false;
	// 连接提示字体
	CFont m_tipFont;
	// 里程计字体
	CFont m_odomFont;
	// 摇杆字体
	CFont m_joyFont;

	// 参数非手动修改标志
	BOOL m_arrAutoModify[4] = { FALSE };
	// 指令控制线程句柄
	HANDLE m_hInstructionThread = NULL;
	// 指令正在执行标志
	bool m_bExecuting = false;
	// sdk时间参数标志
	bool m_arrTimeParam[INST_NUM] = { false };
	// 输入合法标志
	bool m_bInputLegal = 0;

	// 实时控制模式速度
	double m_dLSpeed = 0;
	// 方向
	double m_dASpeed = 0;
	// 实时控制线程句柄
	HANDLE m_hDriveThread = NULL;

	// 阻塞指令进度
	double m_dProgress = 0;
	// 进度条执行标志
	bool m_bProgress = true;
	// 终止标志
	bool m_bAbort = false;

	// 方向键按下状态
	bool m_bUp = false;
	bool m_bDown = false;
	bool m_bLeft = false;
	bool m_bRight = false;
	bool m_bDirectionKey = false;

public:
	// 绘制摇杆
	void DrawJoystick(CDC* pDC);
	// 设置参数界面
	void SetParamView(CString name1, CString unit1,
		CString name2, CString unit2, BOOL radius);
	// 设置文本框内容(避免文本变化事件)
	void SetEditText(int index, CString text);
	// 计算slider与值的比值
	double CalcSliderK(double min, double max);
	// 切换到直行指令
	void SwitchToLine();
	// 切换到转弯指令
	void SwitchToArc();
	// 切换到原地转指令
	void SwitchToAround();
	// 显示指令界面
	void ShowInstruction(BOOL bShow);
	// 刷新运动示意图
	void RefreshBitmap(int select, double speed, double radius = 0);

	// 启动实时控制线程
	void StartDrive();

	// 设置连接提示信息
	void SetTipText(CString text, BOOL warn = FALSE);

	// 设置状态
	void SetState(PM1State state);
	// 刷新状态显示
	void RefreshState();

	// 文本框文本变化响应
	void OnEnChangeEdit(CMyEdit* pEdit, CSliderCtrl* pSlider, int index);

	// 连接
	void Connect();
	// 断开连接
	void Disconnect();

public:
	afx_msg void OnBnClickedBtnClose();
	afx_msg void OnBnClickedBtnMinimize();
	afx_msg void OnBnClickedBtnAbout();
	afx_msg void OnCbnSelchangeCbInstruction();
	afx_msg void OnEnChangeEditV();
	afx_msg void OnEnChangeEditS();
	afx_msg void OnEnChangeEditT();
	afx_msg void OnEnChangeEditR();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedBtnGo();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedBtnInstruction();
	afx_msg void OnBnClickedBtnRealtime();
	afx_msg void OnBnClickedBtnOpen();
	afx_msg void OnCbnSelchangeCbCom();
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnAbort();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnCbnDropdownCbCom();
	afx_msg void OnNMClickSyslinkClear(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnStnClickedPicState();
	afx_msg void OnStnClickedPicTip();
	afx_msg void OnStnDblclickPicTip();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
};
