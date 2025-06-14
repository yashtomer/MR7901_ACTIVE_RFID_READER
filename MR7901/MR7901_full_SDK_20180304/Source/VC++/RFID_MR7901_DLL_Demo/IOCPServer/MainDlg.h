// MainDlg.h : ͷ�ļ�
//

#pragma once

#include "IOCPModel.h"

// CMainDlg �Ի���
class CMainDlg : public CDialog
{
// ����
public:
	CMainDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_IOCPSERVER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	// ��ʼ����
	afx_msg void OnBnClickedStart();
	// ֹͣ����
	afx_msg void OnBnClickedStop();
	// "�˳�"��ť
	afx_msg void OnBnClickedCancel();
	// ��Ӣ���л�
	afx_msg void OnLangChinese();
	afx_msg void OnLangEnglish();
	// ���List�ؼ�
	afx_msg void OnBnClickedButtonCleanList();
	afx_msg LRESULT OnMsgRecvTagData(WPARAM wParam, LPARAM lParam);
	///////////////////////////////////////////////////////////////////////
	//	ϵͳ�˳���ʱ��Ϊȷ����Դ�ͷţ�ֹͣ���������Socket���
	afx_msg void OnDestroy();
	//afx_msg LRESULT OnNewMsg(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:

	// ��ʼ��Socket���Լ�������Ϣ
	void Init();

	// ��ʼ��List�ؼ�
	void InitListCtrl();

	// ���������ļ�
	void InitConfigFile();
	bool MakeResource(int nID, CString szPathName);
	void LoadLanguage(CString szLangSel = "");

	// ��Ӣ���л�
	void LanguageSwitch(int switch_id);

public:

	// ��ǰ�ͻ���������Ϣ������ʱ��������������ʾ�µ�������Ϣ(����CIOCPModel�е���)
	// Ϊ�˼��ٽ�������Ч�ʵ�Ӱ�죬�˴�ʹ��������
	inline void AddInformation(const CString strInfo)
	{
		//CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_DEBUG_INFO);
		m_ListCtrl2.InsertItem(0,strInfo);
	}

private:

	CIOCPModel m_IOCP;                         // ��Ҫ������ɶ˿�ģ��
	CListCtrl  m_ListInfoCtrl;
	CListCtrl  m_ListCtrl2;
	CMenu	   m_menuMain;

public:
	bool       m_bChineseLanguage;	
};
