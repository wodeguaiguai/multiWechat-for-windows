
// multiWechatDlg.h: 头文件
//

#pragma once

#define   WM_SYSTEMTRAY WM_USER+5

// CmultiWechatDlg 对话框
class CmultiWechatDlg : public CDialogEx
{
// 构造
public:
	CmultiWechatDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MULTIWECHAT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:
	CString m_strWechatPath;
	NOTIFYICONDATA m_notifyIcon;

	void LoadWechatPath();

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void StartWechat();
	afx_msg void OnDestroy();
	void CheckAutoStart();
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnAllExit();
	afx_msg void OnBnClickedCheckAutostart();
	CButton m_autoStart;
	CStatic m_tips;
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
