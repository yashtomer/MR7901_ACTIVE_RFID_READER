
#include "stdafx.h"
#include "Languages.h"
#include "IOCPServer.h"
#include "MainDlg.h"
#include "ElectrocarProtocol.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CString g_szSettingPath;	//保存设置的文件路径


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CMainDlg 对话框




CMainDlg::CMainDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMainDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bChineseLanguage = true;
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_INFO, m_ListInfoCtrl);
	DDX_Control(pDX, IDC_LIST_DEBUG_INFO, m_ListCtrl2);
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_START, &CMainDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_STOP, &CMainDlg::OnBnClickedStop)
	ON_WM_CLOSE()
	//ON_MESSAGE(WM_MSG_NEW_MSG,OnNewMsg)
	ON_BN_CLICKED(IDCANCEL, &CMainDlg::OnBnClickedCancel)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_RECV_TAGDATA, OnMsgRecvTagData)
	ON_BN_CLICKED(IDC_BUTTON_CLEAN_LIST, &CMainDlg::OnBnClickedButtonCleanList)
	ON_COMMAND(ID_LANG_CHINESE, &CMainDlg::OnLangChinese)
	ON_COMMAND(ID_LANG_ENGLISH, &CMainDlg::OnLangEnglish)
END_MESSAGE_MAP()


// CMainDlg 消息处理程序

BOOL CMainDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// 设置菜单栏
	m_menuMain.LoadMenu(IDR_MENU_MAIN);
	SetMenu(&m_menuMain);

	// 初始化界面信息
	this->Init();
	ElectrocarProtocol::m_MainDlgHwnd = this->GetSafeHwnd();

	InitConfigFile();
	//装入语言
	LoadLanguage();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMainDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMainDlg::OnPaint()
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
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMainDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//////////////////////////////////////////////////////////////////////
// 初始化Socket库以及界面信息
void CMainDlg::Init()
{
	// 初始化Socket库
	if( false==m_IOCP.LoadSocketLib() )
	{
		AfxMessageBox(_T("Load Winsocket 2.2 failed，server cannot run！"));
		PostQuitMessage(0);
	}

	// 设置本机IP地址
	SetDlgItemText(IDC_SERVERIP, m_IOCP.GetLocalIP());
	// 设置默认端口
	SetDlgItemInt( IDC_EDIT_PORT,DEFAULT_PORT );
	// 初始化列表
	this->InitListCtrl();
	// 绑定主界面指针(为了方便在界面中显示信息 )
	m_IOCP.SetMainDlg(this);

	// 
}

///////////////////////////////////////////////////////////////////////
//	开始监听
void CMainDlg::OnBnClickedStart()
{
	// 设置端口IP地址
	CString strIP;
	GetDlgItemText(IDC_SERVERIP, strIP);
	g_serverIP = strIP.GetBuffer(0);
	strIP.ReleaseBuffer();
	g_nPort = GetDlgItemInt(IDC_EDIT_PORT);
	m_IOCP.SetPort( g_nPort );

	if( false==m_IOCP.Start() )
	{
		AfxMessageBox(_T("Start Server failed！"));
		return;	
	}

	GetDlgItem(IDC_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_STOP)->EnableWindow(TRUE);
}

//////////////////////////////////////////////////////////////////////
//	结束监听
void CMainDlg::OnBnClickedStop()
{
	m_IOCP.Stop();

	GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
	GetDlgItem(IDC_START)->EnableWindow(TRUE);
}

///////////////////////////////////////////////////////////////////////
//	初始化List Control
void CMainDlg::InitListCtrl()
{
	//CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_DEBUG_INFO);
	m_ListCtrl2.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_ListCtrl2.InsertColumn(0, _T("服务器信息"), LVCFMT_LEFT, 495);

	m_ListInfoCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	m_ListInfoCtrl.InsertColumn(1,_T("序号"),LVCFMT_CENTER,50);
	m_ListInfoCtrl.InsertColumn(2,_T("标签编号"),LVCFMT_CENTER,100);
	m_ListInfoCtrl.InsertColumn(3,_T("标签类型"),LVCFMT_CENTER,100);
	m_ListInfoCtrl.InsertColumn(4,_T("采集设备"),LVCFMT_CENTER,150);
	m_ListInfoCtrl.InsertColumn(5,_T("信号强度"),LVCFMT_CENTER,70);
	m_ListInfoCtrl.InsertColumn(6,_T("天线号"),LVCFMT_CENTER,70);
	m_ListInfoCtrl.InsertColumn(7,_T("停留标志"),LVCFMT_CENTER,80);
	m_ListInfoCtrl.InsertColumn(8,_T("进出基站"),LVCFMT_CENTER,80);
	m_ListInfoCtrl.InsertColumn(9,_T("标签采集时间"),LVCFMT_CENTER,145);
}

void CMainDlg::InitConfigFile()
{
	//获取当前路径
	CString szCurPath("");
	GetModuleFileName(NULL, szCurPath.GetBuffer(MAX_PATH), MAX_PATH);
	szCurPath.ReleaseBuffer();
	szCurPath = szCurPath.Left(szCurPath.ReverseFind('\\') + 1);

	//设置文件在当前路径下 
	g_szSettingPath = szCurPath + "setting.ini";
	CFileFind find;
	if (!find.FindFile(g_szSettingPath))
	{
		MakeResource(IDR_SETTING, g_szSettingPath);
	}
	find.Close();
}

bool CMainDlg::MakeResource(int nID, CString szPathName)
{
	HRSRC hSrc = FindResource(NULL, MAKEINTRESOURCE(nID), _T("OWNER_DATA"));
	if (hSrc == NULL)	return false;

	HGLOBAL hGlobal = LoadResource(NULL, hSrc);
	if (hGlobal == NULL)	return false;

	LPVOID lp = LockResource(hGlobal);
	DWORD dwSize = SizeofResource(NULL, hSrc);

	CFile file;
	if (file.Open(szPathName, CFile::modeCreate | CFile::modeWrite))
	{
		file.Write(lp, dwSize);
		file.Close();
	}
	FreeResource(hGlobal);

	return true;
}

/*********************************************************************
* 函数名称:LoadLanguage
* 说明:	装入或者设置语言 如果 szLangSel为空则装入语言，否则设置语言
*********************************************************************/
void CMainDlg::LoadLanguage(CString szLangSel /*= ""*/)
{
	//查找语言
	CString szSection = "Setting";
	CString szKey = "Language", szLang;
	DWORD dwSize = 1000;

	if (!szLangSel.IsEmpty())	//保存语言
	{
		szLang = szLangSel;
		//保存语言设置
		WritePrivateProfileString(szSection, szKey, szLang, g_szSettingPath);
	}
	else	//读取语言设置
	{
		//获取设置的语言
		GetPrivateProfileString(szSection, szKey, "English", szLang.GetBuffer(dwSize), dwSize, g_szSettingPath);
		szLang.ReleaseBuffer();

		if (0 == szLang.Compare("Chinese"))
		{
			OnLangChinese();
		}
		else
		{
			OnLangEnglish();
		}
	}
}

void CMainDlg::OnLangChinese()
{
	LanguageSwitch(1);
	m_IOCP.SetLanguage(1);
	if (!m_bChineseLanguage)
	{
		m_bChineseLanguage = true;
		OnBnClickedButtonCleanList();
		// 保持语言
		LoadLanguage("Chinese");
	}
}

void CMainDlg::OnLangEnglish()
{
	LanguageSwitch(2);
	m_IOCP.SetLanguage(2);
	if (m_bChineseLanguage)
	{
		m_bChineseLanguage = false;
		OnBnClickedButtonCleanList();
		// 保持语言
		LoadLanguage("English");
	}
}

///////////////////////////////////////////////////////////////////////
//	点击“退出”的时候，停止监听，清空Socket类库
void CMainDlg::OnBnClickedCancel()
{
	// 停止监听
	m_IOCP.Stop();

	CDialog::OnCancel();
}

///////////////////////////////////////////////////////////////////////
//	系统退出的时候，为确保资源释放，停止监听，清空Socket类库
void CMainDlg::OnDestroy()
{
	OnBnClickedCancel();

	CDialog::OnDestroy();
}

// 清空控件信息
void CMainDlg::OnBnClickedButtonCleanList()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_DEBUG_INFO);
	pList->DeleteAllItems();
	m_ListInfoCtrl.DeleteAllItems();

	EnterCriticalSection(&m_csTagMsgData);
	TagMsg_Data.clear();
	LeaveCriticalSection(&m_csTagMsgData);
}

LRESULT CMainDlg::OnMsgRecvTagData(WPARAM wParam,LPARAM lParam)
{
	int type_mes=int(wParam);
	unsigned long tagid = (unsigned long)lParam;
	switch(type_mes)
	{
	case 0:
		{
			static long row = 0, sn = 0;
			static CString cstr;
			static TagInfo taginfo;
			EnterCriticalSection(&m_csTagMsgData);
			try
			{
				TAGMSGDATA_MAP::iterator it = TagMsg_Data.find(tagid);
				if (it != TagMsg_Data.end())
				{
					taginfo = it->second;
					row = taginfo.index;
					sn = row + 1;
					if (sn > m_ListInfoCtrl.GetItemCount())
					{
						m_ListInfoCtrl.InsertItem(row, taginfo.szTagID); // 插入行
						cstr.Format(_T("%d"), sn);
						m_ListInfoCtrl.SetItemText(row, 0, cstr); //行号
					}
					m_ListInfoCtrl.SetItemText(row, 1, taginfo.szTagID);
					if (taginfo.nTlvType == 1) {
						if (m_bChineseLanguage) {
							m_ListInfoCtrl.SetItemText(row, 2, Electronic_tag_ch);
						} else {
							m_ListInfoCtrl.SetItemText(row, 2, Electronic_tag_en);
						}
					} else if (taginfo.nTlvType == 2) {
						if (m_bChineseLanguage) {
							m_ListInfoCtrl.SetItemText(row, 2, Attendance_tag_ch);
						} else {
							m_ListInfoCtrl.SetItemText(row, 2, Attendance_tag_en);
						}
					}
					m_ListInfoCtrl.SetItemText(row, 3, taginfo.szSerial);
					cstr.Format(_T("%d"), taginfo.intensity);
					m_ListInfoCtrl.SetItemText(row, 4, cstr);
					cstr.Format(_T("%d"), taginfo.ichannelid);
					m_ListInfoCtrl.SetItemText(row, 5, cstr);
					if (taginfo.iStay_flag == 1)
					{
						m_ListInfoCtrl.SetItemText(row, 6, _T("1"));
						if (m_bChineseLanguage) {
							m_ListInfoCtrl.SetItemText(row, 7, Staying_flag_ch);
						}else {
							m_ListInfoCtrl.SetItemText(row, 7, Staying_flag_en);
						}
					}
					else
					{
						m_ListInfoCtrl.SetItemText(row, 6, _T("0"));
						if (taginfo.inout_type == 1) {
							if (m_bChineseLanguage) {
								m_ListInfoCtrl.SetItemText(row, 7, Entry_ch);
							} else {
								m_ListInfoCtrl.SetItemText(row, 7, Entry_en);
							}
						}
						else 
						{
							if (m_bChineseLanguage) {
								m_ListInfoCtrl.SetItemText(row, 7, Out_ch);
							} else {
								m_ListInfoCtrl.SetItemText(row, 7, Out_en);
							}
						}
					}
					m_ListInfoCtrl.SetItemText(row, 8, taginfo.szTagTime);
				}

				if(m_ListInfoCtrl.GetItemCount() >= 5000)
				{
					m_ListInfoCtrl.DeleteAllItems();
					EnterCriticalSection(&m_csTagMsgData);
					TagMsg_Data.clear();
					LeaveCriticalSection(&m_csTagMsgData);
				}
			}catch(...){
				;
			}
			LeaveCriticalSection(&m_csTagMsgData);
		}
		break;
	default:
		break;
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// 中英文切换 1-中文，2-英文
void CMainDlg::LanguageSwitch(int switch_id)
{
	int nCurPos = 0;
	CMenu* subMenu = m_menuMain.GetSubMenu(nCurPos);
	HDITEM htItem = { 0 };
	htItem.mask = HDI_TEXT;

	switch (switch_id)
	{
	case 1:
	    {
			// 菜单
			m_menuMain.ModifyMenu(nCurPos, MF_BYPOSITION, nCurPos, Language_ch);
			subMenu->ModifyMenu(ID_LANG_CHINESE, MF_BYCOMMAND, ID_LANG_CHINESE, Lang_chinese_ch);
			subMenu->ModifyMenu(ID_LANG_ENGLISH, MF_BYCOMMAND, ID_LANG_ENGLISH, Lang_english_ch);
			// 文字
			SetWindowText(Title_name_ch);
			GetDlgItem(IDC_STATIC1)->SetWindowText(ServerIP_addr_ch);
			GetDlgItem(IDC_STATIC2)->SetWindowText(Server_Port_ch);
			GetDlgItem(IDC_BUTTON_CLEAN_LIST)->SetWindowText(Clear_ListCtrl_ch);
			GetDlgItem(IDC_START)->SetWindowText(Start_listen_ch);
			GetDlgItem(IDC_STOP)->SetWindowText(Stop_listen_ch);
			GetDlgItem(IDCANCEL)->SetWindowText(Exist_system_ch);
			// 列表控件2
			htItem.pszText = Server_Info_ch;
			m_ListCtrl2.GetHeaderCtrl()->SetItem(0, &htItem);
			// 列表控件1
			htItem.pszText = SN_number_ch;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(0, &htItem);
			htItem.pszText = Tag_ID_ch;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(1, &htItem);
			htItem.pszText = Tag_Type_ch;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(2, &htItem);
			htItem.pszText = Signal_Collector_ch;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(3, &htItem);
			htItem.pszText = RSSI_info_ch;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(4, &htItem);
			htItem.pszText = Antenna_Number_ch;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(5, &htItem);
			htItem.pszText = Staying_Indicate_ch;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(6, &htItem);
			htItem.pszText = Entry_Or_Exit_ch;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(7, &htItem);
			htItem.pszText = Label_Time_ch;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(8, &htItem);
	    }
		break;
	case 2:
	    {
			// 菜单
			m_menuMain.ModifyMenu(nCurPos, MF_BYPOSITION, nCurPos, Language_en);
			subMenu->ModifyMenu(ID_LANG_CHINESE, MF_BYCOMMAND, ID_LANG_CHINESE, Lang_chinese_en);
			subMenu->ModifyMenu(ID_LANG_ENGLISH, MF_BYCOMMAND, ID_LANG_ENGLISH, Lang_english_en);
			// 文字
			SetWindowText(Title_name_en);
			GetDlgItem(IDC_STATIC1)->SetWindowText(ServerIP_addr_en);
			GetDlgItem(IDC_STATIC2)->SetWindowText(Server_Port_en);
			GetDlgItem(IDC_BUTTON_CLEAN_LIST)->SetWindowText(Clear_ListCtrl_en);
			GetDlgItem(IDC_START)->SetWindowText(Start_listen_en);
			GetDlgItem(IDC_STOP)->SetWindowText(Stop_listen_en);
			GetDlgItem(IDCANCEL)->SetWindowText(Exist_system_en);
			// 列表控件2
			htItem.pszText = Server_Info_en;
			m_ListCtrl2.GetHeaderCtrl()->SetItem(0, &htItem);
			// 列表控件1
			htItem.pszText = SN_number_en;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(0, &htItem);
			htItem.pszText = Tag_ID_en;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(1, &htItem);
			htItem.pszText = Tag_Type_en;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(2, &htItem);
			htItem.pszText = Signal_Collector_en;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(3, &htItem);
			htItem.pszText = RSSI_info_en;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(4, &htItem);
			htItem.pszText = Antenna_Number_en;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(5, &htItem);
			htItem.pszText = Staying_Indicate_en;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(6, &htItem);
			htItem.pszText = Entry_Or_Exit_en;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(7, &htItem);
			htItem.pszText = Label_Time_en;
			m_ListInfoCtrl.GetHeaderCtrl()->SetItem(8, &htItem);
	    }
		break;
	default:
		break;
	}

	DrawMenuBar();		//刷新菜单显示
	Invalidate(false);	//刷新对话框的语言显示
}
