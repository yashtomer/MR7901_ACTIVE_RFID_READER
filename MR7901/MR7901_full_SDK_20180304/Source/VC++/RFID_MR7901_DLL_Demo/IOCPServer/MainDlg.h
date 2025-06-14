// MainDlg.h : 头文件
//

#pragma once

#include "IOCPModel.h"

// CMainDlg 对话框
class CMainDlg : public CDialog
{
// 构造
public:
	CMainDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_IOCPSERVER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	// 开始监听
	afx_msg void OnBnClickedStart();
	// 停止监听
	afx_msg void OnBnClickedStop();
	// "退出"按钮
	afx_msg void OnBnClickedCancel();
	// 中英文切换
	afx_msg void OnLangChinese();
	afx_msg void OnLangEnglish();
	// 清空List控件
	afx_msg void OnBnClickedButtonCleanList();
	afx_msg LRESULT OnMsgRecvTagData(WPARAM wParam, LPARAM lParam);
	///////////////////////////////////////////////////////////////////////
	//	系统退出的时候，为确保资源释放，停止监听，清空Socket类库
	afx_msg void OnDestroy();
	//afx_msg LRESULT OnNewMsg(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:

	// 初始化Socket库以及界面信息
	void Init();

	// 初始化List控件
	void InitListCtrl();

	// 加载配置文件
	void InitConfigFile();
	bool MakeResource(int nID, CString szPathName);
	void LoadLanguage(CString szLangSel = "");

	// 中英文切换
	void LanguageSwitch(int switch_id);

public:

	// 当前客户端有新消息到来的时候，在主界面中显示新到来的信息(在类CIOCPModel中调用)
	// 为了减少界面代码对效率的影响，此处使用了内联
	inline void AddInformation(const CString strInfo)
	{
		//CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_DEBUG_INFO);
		m_ListCtrl2.InsertItem(0,strInfo);
	}

private:

	CIOCPModel m_IOCP;                         // 主要对象，完成端口模型
	CListCtrl  m_ListInfoCtrl;
	CListCtrl  m_ListCtrl2;
	CMenu	   m_menuMain;

public:
	bool       m_bChineseLanguage;	
};
