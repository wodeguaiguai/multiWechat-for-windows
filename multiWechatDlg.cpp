
// multiWechatDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "multiWechat.h"
#include "multiWechatDlg.h"
#include "afxdialogex.h"
#include <aclapi.h>
#include <shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CmultiWechatDlg 对话框



CmultiWechatDlg::CmultiWechatDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MULTIWECHAT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_WECHAT);
}

void CmultiWechatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_AUTOSTART, m_autoStart);
}

BEGIN_MESSAGE_MAP(CmultiWechatDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_STAR, &CmultiWechatDlg::StartWechat)
	ON_WM_DESTROY()
	ON_COMMAND(ID_ALL_EXIT, &CmultiWechatDlg::OnAllExit)
	ON_BN_CLICKED(IDC_CHECK_AUTOSTART, &CmultiWechatDlg::OnBnClickedCheckAutostart)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CmultiWechatDlg 消息处理程序

void OutPutString(LPCWSTR strFormat, ...)
{
	if (NULL == strFormat)
	{
		return;
	}
	TCHAR strInfo[4096] = { 0 };
	va_list arg_ptr = NULL;
	va_start(arg_ptr, strFormat);
	vswprintf_s(strInfo, 4096, strFormat, arg_ptr);
	va_end(arg_ptr);
	OutputDebugString(strInfo);
}

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;
BOOL IsWow64()
{
	BOOL bIsWow64 = FALSE;
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), 
		"IsWow64Process");
	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			return TRUE;
		}
	}
	return bIsWow64;
}

void EnableMultiWeChat()
{
	HANDLE hMutex = CreateMutexW(NULL, FALSE, L"_WeChat_App_Instance_Identity_Mutex_Name");
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	PSID pEveryoneSID = NULL;
	char szBuffer[4096] = {0};
	PACL pAcl = (PACL)szBuffer;

	AllocateAndInitializeSid(
		&SIDAuthWorld,
		1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
		&pEveryoneSID);

	InitializeAcl(pAcl, sizeof(szBuffer), ACL_REVISION);
	AddAccessDeniedAce(pAcl, ACL_REVISION, MUTEX_ALL_ACCESS, pEveryoneSID);
	SetSecurityInfo(hMutex, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL);
}

CString ReadRegValue(
	_In_ HKEY hKey, 
	_In_opt_ LPCWSTR lpSubKey, 
	_In_opt_ LPCWSTR lpValueName, 
	DWORD *pdwTpye)
{
	HKEY hHandle = nullptr;
	if (ERROR_SUCCESS != RegOpenKeyEx(hKey, lpSubKey, 0, KEY_READ, &hHandle))
	{
		return L"";
	}

	DWORD dwValue = MAX_PATH;
	TCHAR szWechatPath[MAX_PATH + 1] = { 0 };
	if (ERROR_SUCCESS != RegQueryValueEx(hHandle, lpValueName, 0, pdwTpye, (LPBYTE)szWechatPath, &dwValue))
	{
		CloseHandle(hHandle);
		return L"";
	}
	CloseHandle(hHandle);

	return CString(szWechatPath);
}

void DeleteRegValue(
	_In_ HKEY hKey,
	_In_opt_ LPCWSTR lpSubKey,
	_In_opt_ LPCWSTR lpValueName)
{
	HKEY hHandle = nullptr;
	if (ERROR_SUCCESS != RegOpenKeyEx(hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hHandle))
	{
		return;
	}

	if (ERROR_SUCCESS != RegDeleteValue(hHandle, lpValueName))
	{
		AfxMessageBox(L"注册表删除失败！");
	}
	CloseHandle(hHandle);
}

void WriteRegValue(
	_In_ HKEY hKey,
	_In_opt_ LPCWSTR lpSubKey,
	_In_opt_ LPCWSTR lpValueName,
	_In_opt_ DWORD      dwType,
	_In_opt_ const BYTE* lpData,
	_In_opt_ DWORD      cbData
	)
{
	HKEY hHandle = nullptr;
	if (ERROR_SUCCESS != RegOpenKeyEx(hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hHandle))
	{
		return;
	}

	if (ERROR_SUCCESS != RegSetValueEx(hHandle, lpValueName, 0, dwType, lpData, cbData))
	{
		AfxMessageBox(L"注册表写入失败！");
	}
	RegFlushKey(hHandle);
	CloseHandle(hHandle);
}

void CmultiWechatDlg::LoadWechatPath()
{
	DWORD dwTpye = 0;
	if (IsWow64()) {
		m_strWechatPath = ReadRegValue(
			HKEY_LOCAL_MACHINE,
			TEXT("SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\WeChat"),
			TEXT("DisplayIcon"),
			&dwTpye);
	}
	else {
		m_strWechatPath = ReadRegValue(
			HKEY_LOCAL_MACHINE,
			TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\WeChat"),
			TEXT("DisplayIcon"),
			&dwTpye);
	}
	m_strWechatPath.TrimLeft(L"\"");
	m_strWechatPath.TrimRight(L"\"");
}

void CmultiWechatDlg::StartWechat()
{	
	if (!PathFileExists(m_strWechatPath)) {
		AfxMessageBox(TEXT("本机微信未安装!"));
	}
	else {
		ShellExecute(NULL, TEXT("open"), m_strWechatPath, NULL, NULL, SW_SHOWNORMAL);
	}
}


void CmultiWechatDlg::CheckAutoStart()
{
	DWORD dwTpye = 0;
	CString strStartString;
	if (IsWow64()) 
	{
		strStartString = ReadRegValue(
			HKEY_LOCAL_MACHINE,
			TEXT("SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run\\"),
			TEXT("MultiWechat"),
			&dwTpye);
	}
	else 
	{
		strStartString = ReadRegValue(
			HKEY_LOCAL_MACHINE,
			TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\"),
			TEXT("MultiWechat"),
			&dwTpye);
	}
	if (strStartString.GetLength() > 0) {
		m_autoStart.SetCheck(1);
	}
}

BOOL CmultiWechatDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	//ShowWindow(SW_MINIMIZE);

	// TODO: 在此添加额外的初始化代码

	EnableMultiWeChat();

	CheckAutoStart();

	LoadWechatPath();

	m_notifyIcon.cbSize = sizeof(NOTIFYICONDATA);
	m_notifyIcon.hIcon = m_hIcon;
	m_notifyIcon.hWnd = m_hWnd;
	lstrcpy(m_notifyIcon.szTip, _T("微信多开小助手"));
	m_notifyIcon.uCallbackMessage = WM_SYSTEMTRAY;
	m_notifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	Shell_NotifyIcon(NIM_ADD, &m_notifyIcon);
;
	int nArgs;
	LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (nArgs >= 2)
	{
		for (size_t i = 0; i < nArgs; i++)
		{
			if (lstrcmpW(szArglist[1], L"-backgroud") == 0) {
				PostMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
				break;
			}
		}
	}

	LocalFree(szArglist);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CmultiWechatDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CmultiWechatDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CmultiWechatDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CmultiWechatDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	Shell_NotifyIcon(NIM_DELETE, &m_notifyIcon);
}

LRESULT CmultiWechatDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: 在此添加专用代码和/或调用基类
	switch (message) {
	case WM_SYSTEMTRAY:
		if (lParam == WM_LBUTTONDBLCLK)
		{
			if (AfxGetApp()->m_pMainWnd->IsWindowVisible())
			{
				AfxGetApp()->m_pMainWnd->ShowWindow(SW_HIDE);
			}
			else
			{
				AfxGetApp()->m_pMainWnd->ShowWindow(SW_SHOW);
				CenterWindow();
			}
		}
		else if (lParam == WM_RBUTTONDOWN)
		{
			CMenu menu;
			menu.LoadMenu(IDR_MENU_EXIT);
			CMenu* pMenu = menu.GetSubMenu(0);
			CPoint pos;
			GetCursorPos(&pos);
			pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, AfxGetMainWnd());
		}
		break;
	case WM_DESTROY:
		::Shell_NotifyIcon(NIM_DELETE, &m_notifyIcon);
		break;
	case WM_SYSCOMMAND:
		if (wParam == SC_MINIMIZE)
		{
			AfxGetApp()->m_pMainWnd->ShowWindow(SW_HIDE);
			return 0;
		}
		break;
	}

	return CDialogEx::WindowProc(message, wParam, lParam);
}


void CmultiWechatDlg::OnAllExit()
{
	PostMessage(WM_QUIT, 0, 0);
}

void CmultiWechatDlg::OnBnClickedCheckAutostart()
{
	CString strKey = TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run\\");
	if (!IsWow64()) {
		strKey = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\");
	}
	CString strItemName = TEXT("MultiWechat");
	if (m_autoStart.GetCheck()) {
		TCHAR szPath[MAX_PATH + 1] = { 0 };
		GetModuleFileName(NULL, szPath, MAX_PATH);
		CString strPath;
		strPath.Format(L"\"%s\" -background", szPath);
		WriteRegValue(
			HKEY_LOCAL_MACHINE,
			strKey,
			strItemName,
			REG_SZ,
			(const BYTE*)strPath.GetBuffer(0),
			strPath.GetLength() * 2);
		strPath.ReleaseBuffer();
	}
	else {
		DeleteRegValue(
			HKEY_LOCAL_MACHINE,
			strKey,
			strItemName);
	}
}


HBRUSH CmultiWechatDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_TIPS) 
	{
		pDC->SetTextColor(RGB(255, 0, 0));
	}

	return hbr;
}