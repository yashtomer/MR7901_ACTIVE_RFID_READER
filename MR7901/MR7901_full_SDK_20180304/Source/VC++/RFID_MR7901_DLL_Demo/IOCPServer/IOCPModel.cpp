#include "StdAfx.h"
#include "IOCPModel.h"
#include "MainDlg.h"
#include "ElectrocarProtocol.h"

// ÿһ���������ϲ������ٸ��߳�
#define WORKER_THREADS_PER_PROCESSOR 2
// ͬʱͶ�ݵ�AcceptEx���������
#define MAX_POST_ACCEPT              10
// ���ݸ�Worker�̵߳��˳��ź�
#define EXIT_CODE                    NULL


// �ͷ�ָ��;����Դ�ĺ�

// �ͷ�ָ���
#define RELEASE(x)                      {if(x != NULL ){delete x;x=NULL;}}
// �ͷž����
#define RELEASE_HANDLE(x)               {if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x = NULL;}}
// �ͷ�Socket��
#define RELEASE_SOCKET(x)               {if(x !=INVALID_SOCKET) { closesocket(x);x=INVALID_SOCKET;}}

bool CIOCPModel::m_bChineseLang = true;


CIOCPModel::CIOCPModel(void):
							m_nThreads(0),
							m_hShutdownEvent(NULL),
							m_hIOCompletionPort(NULL),
							m_phWorkerThreads(NULL),
							m_strIP(DEFAULT_IP),
							m_nPort(DEFAULT_PORT),
							m_pMain(NULL),
							m_lpfnAcceptEx( NULL ),
							m_pListenContext( NULL )
{
	// ��ʼ���̻߳�����
	InitializeCriticalSection(&m_csContextList);
	InitializeCriticalSection(&m_csTagMsgData);
}


CIOCPModel::~CIOCPModel(void)
{
	// ȷ����Դ�����ͷ�
	this->Stop();

	// ɾ���ͻ����б�Ļ�����
	DeleteCriticalSection(&m_csContextList);
	DeleteCriticalSection(&m_csTagMsgData);
}


/*********************************************************************
*�������ܣ��̺߳���������GetQueuedCompletionStatus����������д���
*����������lpParam��THREADPARAMS_WORKER����ָ�룻
*����˵����GetQueuedCompletionStatus��ȷ����ʱ��ʾĳ�����Ѿ���ɣ��ڶ�������lpNumberOfBytes��ʾ�����׽��ִ�����ֽ�����
����lpCompletionKey��lpOverlapped������Ҫ����Ϣ�����ѯMSDN�ĵ���
*********************************************************************/
DWORD WINAPI CIOCPModel::_WorkerThread(LPVOID lpParam)
{
	THREADPARAMS_WORKER* pParam = (THREADPARAMS_WORKER*)lpParam;
	CIOCPModel* pIOCPModel = (CIOCPModel*)pParam->pIOCPModel;
	int nThreadNo = (int)pParam->nThreadNo;

	//pIOCPModel->_ShowMessage("Start work thread��ID: %d.",nThreadNo);

	OVERLAPPED           *pOverlapped = NULL;
	PER_SOCKET_CONTEXT   *pSocketContext = NULL;
	DWORD                dwBytesTransfered = 0;

	//ѭ����������ֱ�����յ�Shutdown��ϢΪֹ
	while (WAIT_OBJECT_0 != WaitForSingleObject(pIOCPModel->m_hShutdownEvent, 0))
	{
		BOOL bReturn = GetQueuedCompletionStatus(
			pIOCPModel->m_hIOCompletionPort,
			&dwBytesTransfered,
			(PULONG_PTR)&pSocketContext,
			&pOverlapped,
			INFINITE);

		//����EXIT_CODE�˳���־����ֱ���˳�
		if ( EXIT_CODE==(DWORD)pSocketContext )
		{
			break;
		}

		//����ֵΪ0����ʾ����
		if( !bReturn )  
		{  
			DWORD dwErr = GetLastError();

			// ��ʾһ����ʾ��Ϣ
			if( !pIOCPModel->HandleError( pSocketContext,dwErr ) )
			{
				break;
			}

			continue;  
		}  
		else  
		{
			// ��ȡ����Ĳ���
			PER_IO_CONTEXT* pIoContext = CONTAINING_RECORD(pOverlapped, PER_IO_CONTEXT, m_Overlapped);

			// �ж��Ƿ��пͻ��˶Ͽ���
			if((0 == dwBytesTransfered) && ( RECV_POSTED==pIoContext->m_OpType || SEND_POSTED==pIoContext->m_OpType))  
			{
				if (m_bChineseLang)
					pIOCPModel->_ShowMessage( _T("�ͻ��� %s:%d �Ͽ�����.  %s"),inet_ntoa(pSocketContext->m_ClientAddr.sin_addr), ntohs(pSocketContext->m_ClientAddr.sin_port), pIoContext->m_szSerial );
				else
					pIOCPModel->_ShowMessage(_T("Client %s:%d Disconnect.  %s"), inet_ntoa(pSocketContext->m_ClientAddr.sin_addr), ntohs(pSocketContext->m_ClientAddr.sin_port), pIoContext->m_szSerial);

				// �ͷŵ���Ӧ����Դ
				pIOCPModel->_RemoveContext( pSocketContext );

 				continue;  
			}  
			else
			{
				switch( pIoContext->m_OpType )  
				{
					 // Accept  
				case ACCEPT_POSTED:
					{
						pIoContext->m_nTotalBytes = dwBytesTransfered;
						pIOCPModel->_DoAccpet( pSocketContext, pIoContext);						
					}
					break;

					// RECV
				case RECV_POSTED:
					{
						pIoContext->m_nTotalBytes	= dwBytesTransfered;
						pIOCPModel->_DoRecv( pSocketContext,pIoContext );
					}
					break;

				case SEND_POSTED:
					{
						pIoContext->m_nSendBytes += dwBytesTransfered;
						if (pIoContext->m_nSendBytes < pIoContext->m_nTotalBytes)
						{
							//����δ�ܷ����꣬������������
							pIoContext->m_wsaBuf.buf = pIoContext->m_szBuffer + pIoContext->m_nSendBytes;
							pIoContext->m_wsaBuf.len = pIoContext->m_nTotalBytes - pIoContext->m_nSendBytes;
							pIOCPModel->PostWrite(pIoContext);
						}
						else
						{
							pIOCPModel->PostRecv(pIoContext);
						}
					}
					break;
				default:
					// ��Ӧ��ִ�е�����
					TRACE(_T("_WorkThread�е� pIoContext->m_OpType �����쳣.\n"));
					break;
				} //switch
			}//if
		}//if

	}//while

	TRACE(_T("�������߳� %d ���˳�.\n"),nThreadNo);

	// �ͷ��̲߳���
	RELEASE(lpParam);	

	return 0;
}

//�������ܣ���ʼ���׽���
bool CIOCPModel::LoadSocketLib()
{    
	WSADATA wsaData;
	int nResult;
	nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	// ����(һ�㶼�����ܳ���)
	if (NO_ERROR != nResult)
	{
		this->_ShowMessage(_T("WinSocket 2.2 initialization faild��\n"));
		return false; 
	}

	return true;
}


//�������ܣ�����������
bool CIOCPModel::Start()
{
	// ����ϵͳ�˳����¼�֪ͨ
	m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// ��ʼ��IOCP
	if (false == _InitializeIOCP())
	{
		if (m_bChineseLang)
			this->_ShowMessage(_T("��ʼ��IOCPʧ�ܣ�\n"));
		else
			this->_ShowMessage(_T("IOCP initialization failed��\n"));
		return false;
	}
	else
	{
		if (m_bChineseLang)
			this->_ShowMessage("\nIOCP��������ʼ�����.\n.");
		else
			this->_ShowMessage("\nServer initialization completed.\n.");
	}

	// ��ʼ��Socket
	if( false==_InitializeListenSocket() )
	{
		if (m_bChineseLang)
			this->_ShowMessage(_T("Listen Socket��ʼ��ʧ�ܣ�\n"));
		else
			this->_ShowMessage(_T("Listen Socket initialization failed��\n"));
		this->_DeInitialize();
		return false;
	}
	else
	{
		if (m_bChineseLang)
			this->_ShowMessage("Listen Socket��ʼ�����.");
		else
			this->_ShowMessage("Listen Socket initialization completed.");
	}

	if (m_bChineseLang)
		this->_ShowMessage(_T("ϵͳ׼���������Ⱥ�����....\n"));
	else
		this->_ShowMessage(_T("The system is ready to wait for connections....\n"));

	return true;
}


////////////////////////////////////////////////////////////////////
//	��ʼ����ϵͳ�˳���Ϣ���˳���ɶ˿ں��߳���Դ
void CIOCPModel::Stop()
{
	if( m_pListenContext!=NULL && m_pListenContext->m_Socket!=INVALID_SOCKET )
	{
		// ����ر���Ϣ֪ͨ
		SetEvent(m_hShutdownEvent);

		for (int i = 0; i < m_nThreads; i++)
		{
			// ֪ͨ���е���ɶ˿ڲ����˳�
			PostQueuedCompletionStatus(m_hIOCompletionPort, 0, (DWORD)EXIT_CODE, NULL);
		}

		// �ȴ����еĿͻ�����Դ�˳�
		WaitForMultipleObjects(m_nThreads, m_phWorkerThreads, TRUE, INFINITE);

		// ����ͻ����б���Ϣ
		this->_ClearContextList();

		// �ͷ�������Դ
		this->_DeInitialize();

		if (m_bChineseLang)
			this->_ShowMessage("ֹͣ����\n");
		else
			this->_ShowMessage("Stop listening.\n");
	}	
}


void CIOCPModel::SetLanguage(int IDD)
{
	if (1 == IDD)
		m_bChineseLang = true;
	else
		m_bChineseLang = false;
}

/*************************************************************
*�������ܣ�Ͷ��WSARecv����
*����������
PER_IO_CONTEXT* pIoContext:	���ڽ���IO���׽����ϵĽṹ����ҪΪWSARecv������WSASend������
**************************************************************/
bool CIOCPModel::PostRecv( PER_IO_CONTEXT* pIoContext )
{
	// ��ʼ������
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	WSABUF *p_wbuf   = &pIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

	pIoContext->ResetBuffer();
	pIoContext->m_OpType = RECV_POSTED;
	pIoContext->m_nSendBytes = 0;
	pIoContext->m_nTotalBytes= 0;

	// ��ʼ����ɺ󣬣�Ͷ��WSARecv����
	int nBytesRecv = WSARecv( pIoContext->m_sockAccept, p_wbuf, 1, &dwBytes, &dwFlags, p_ol, NULL );

	// �������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		if (m_bChineseLang)
			this->_ShowMessage("Ͷ�ݵ�һ��WSARecvʧ�ܣ�");
		else
			this->_ShowMessage("WSARecv failed��");
		return false;
	}

	return true;
}

/*************************************************************
*�������ܣ�Ͷ��WSASend����
*����������
PER_IO_CONTEXT* pIoContext:	���ڽ���IO���׽����ϵĽṹ����ҪΪWSARecv������WSASend����
*����˵��������PostWrite֮ǰ��Ҫ����pIoContext��m_wsaBuf, m_nTotalBytes, m_nSendBytes��
**************************************************************/
bool CIOCPModel::PostWrite(PER_IO_CONTEXT* pIoContext)
{
	// ��ʼ������
	DWORD dwFlags = 0;
	DWORD dwSendNumBytes = 0;
	WSABUF *p_wbuf   = &pIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

	pIoContext->m_OpType = SEND_POSTED;

	unsigned char head = pIoContext->m_wsaBuf.buf[0];
	unsigned char subhead = pIoContext->m_wsaBuf.buf[1];
	if( RFID_CMD_LOGIN == head && RFID_CMD_LOGIN_AA == subhead )
	{
		//��ʼ�綯�����ݰ����Э��
		bool bResult = ElectrocarProtocol::recv_buffer_parse(pIoContext, pIoContext->m_wsaBuf.buf, pIoContext->m_wsaBuf.len);
		if (!bResult)
		{
			if (m_bChineseLang)
				this->_ShowMessage("Ͷ��WSASendʧ�ܣ�");
			else
				this->_ShowMessage("WSASend failed��");
			return false;
		}
	}

	//Ͷ��WSASend���� -- ��Ҫ�޸�
	int nRet = WSASend(pIoContext->m_sockAccept, &pIoContext->m_wsaBuf, 1, &dwSendNumBytes, dwFlags,
		&pIoContext->m_Overlapped, NULL);

	// �������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����
	if ((SOCKET_ERROR == nRet) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		if (m_bChineseLang)
			this->_ShowMessage("Ͷ��WSASendʧ�ܣ�");
		else
			this->_ShowMessage("WSASend failed��");
		return false;
	}
	return true;
}


////////////////////////////////
// ��ʼ����ɶ˿�
bool CIOCPModel::_InitializeIOCP()
{
	// ������һ����ɶ˿�
	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0 );

	if ( NULL == m_hIOCompletionPort)
	{
		if (m_bChineseLang)
			this->_ShowMessage(_T("������ɶ˿�ʧ�ܣ��������: %d!\n"), WSAGetLastError());
		else
			this->_ShowMessage(_T("Create CompletionPort failed��ErrorCode: %d!\n"), WSAGetLastError());
		return false;
	}

	// ���ݱ����еĴ�����������������Ӧ���߳���
	m_nThreads = WORKER_THREADS_PER_PROCESSOR * _GetNoOfProcessors();
	
	// Ϊ�������̳߳�ʼ�����
	m_phWorkerThreads = new HANDLE[m_nThreads];
	
	// ���ݼ�����������������������߳�
	DWORD nThreadID;
	for (int i = 0; i < m_nThreads; i++)
	{
		THREADPARAMS_WORKER* pThreadParams = new THREADPARAMS_WORKER;
		pThreadParams->pIOCPModel = this;
		pThreadParams->nThreadNo  = i+1;
		m_phWorkerThreads[i] = ::CreateThread(0, 0, _WorkerThread, (void *)pThreadParams, 0, &nThreadID);
	}

	TRACE(" ���� _WorkerThread %d ��.\n", m_nThreads );

	return true;
}


/////////////////////////////////////////////////////////////////
// ��ʼ��Socket
bool CIOCPModel::_InitializeListenSocket()
{
	// AcceptEx �� GetAcceptExSockaddrs ��GUID�����ڵ�������ָ��
	GUID GuidAcceptEx = WSAID_ACCEPTEX;  
	GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS; 

	// ��������ַ��Ϣ�����ڰ�Socket
	struct sockaddr_in ServerAddress;

	// �������ڼ�����Socket����Ϣ
	m_pListenContext = new PER_SOCKET_CONTEXT;

	// ��Ҫʹ���ص�IO�������ʹ��WSASocket������Socket���ſ���֧���ص�IO����
	m_pListenContext->m_Socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_pListenContext->m_Socket) 
	{
		if (m_bChineseLang)
			this->_ShowMessage("��ʼ��Socketʧ�ܣ��������: %d.\n", WSAGetLastError());
		else
			this->_ShowMessage("Socket initialization failed��ErrorCode: %d.\n", WSAGetLastError());
		return false;
	}
	else
	{
		TRACE("WSASocket() ���.\n");
	}

	// ��Listen Socket������ɶ˿���
	if( NULL== CreateIoCompletionPort( (HANDLE)m_pListenContext->m_Socket, m_hIOCompletionPort,(DWORD)m_pListenContext, 0))  
	{
		if (m_bChineseLang)
			this->_ShowMessage("�� Listen Socket����ɶ˿�ʧ�ܣ��������: %d/n", WSAGetLastError()); 
		else
			this->_ShowMessage("Bind Listen Socket failed��ErrorCode: %d/n", WSAGetLastError());
		RELEASE_SOCKET( m_pListenContext->m_Socket );
		return false;
	}
	else
	{
		TRACE("Listen Socket����ɶ˿� ���.\n");
	}

	// ����ַ��Ϣ
	ZeroMemory((char *)&ServerAddress, sizeof(ServerAddress));
	ServerAddress.sin_family = AF_INET;
	// ������԰��κο��õ�IP��ַ�����߰�һ��ָ����IP��ַ 
	//ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	m_strIP = CString("0.0.0.0");
	ServerAddress.sin_addr.s_addr = inet_addr(m_strIP.GetString());         
	ServerAddress.sin_port = htons(m_nPort);                          

	// �󶨵�ַ�Ͷ˿�
	if (SOCKET_ERROR == bind(m_pListenContext->m_Socket, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress))) 
	{
		if (m_bChineseLang)
			this->_ShowMessage("bind()����ִ�д���.\n");
		else
			this->_ShowMessage("bind() function error.\n");
		return false;
	}
	else
	{
		TRACE("bind() ���.\n");
	}

	// ��ʼ���м���
	if (SOCKET_ERROR == listen(m_pListenContext->m_Socket,SOMAXCONN))
	{
		if (m_bChineseLang)
			this->_ShowMessage("listen()����ִ�д���.\n");
		else
			this->_ShowMessage("listen() function error.\n");
		return false;
	}
	else
	{
		TRACE("Listen() ���.\n");
	}

	// ʹ��AcceptEx��������Ϊ���������WinSock2�淶֮���΢�������ṩ����չ����
	// ������Ҫ�����ȡһ�º�����ָ�룬
	// ��ȡAcceptEx����ָ��
	DWORD dwBytes = 0;  
	if(SOCKET_ERROR == WSAIoctl(
		m_pListenContext->m_Socket, 
		SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&GuidAcceptEx, 
		sizeof(GuidAcceptEx), 
		&m_lpfnAcceptEx, 
		sizeof(m_lpfnAcceptEx), 
		&dwBytes, 
		NULL, 
		NULL))  
	{
		if (m_bChineseLang)
			this->_ShowMessage("WSAIoctl δ�ܻ�ȡAcceptEx����ָ�롣�������: %d\n", WSAGetLastError());
		else
			this->_ShowMessage("WSAIoctl failed! ErrorCode: %d\n", WSAGetLastError());
		this->_DeInitialize();
		return false;  
	}  

	// ��ȡGetAcceptExSockAddrs����ָ�룬Ҳ��ͬ��
	if(SOCKET_ERROR == WSAIoctl(
		m_pListenContext->m_Socket, 
		SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&GuidGetAcceptExSockAddrs,
		sizeof(GuidGetAcceptExSockAddrs), 
		&m_lpfnGetAcceptExSockAddrs, 
		sizeof(m_lpfnGetAcceptExSockAddrs),   
		&dwBytes, 
		NULL, 
		NULL))  
	{
		if (m_bChineseLang)
			this->_ShowMessage("WSAIoctl δ�ܻ�ȡGuidGetAcceptExSockAddrs����ָ�롣�������: %d\n", WSAGetLastError());
		else
			this->_ShowMessage("WSAIoctl failed! ErrorCode: %d\n", WSAGetLastError());
		this->_DeInitialize();
		return false; 
	}  


	// ΪAcceptEx ׼��������Ȼ��Ͷ��AcceptEx I/O����
	//����10���׽��֣�Ͷ��AcceptEx���󣬼�����10���׽��ֽ���accept������
	for( int i=0;i<MAX_POST_ACCEPT;i++ )
	{
		// �½�һ��IO_CONTEXT
		PER_IO_CONTEXT* pAcceptIoContext = m_pListenContext->GetNewIoContext();

		if( false==this->_PostAccept( pAcceptIoContext ) )
		{
			m_pListenContext->RemoveContext(pAcceptIoContext);
			return false;
		}
	}

	if (m_bChineseLang)
		this->_ShowMessage( _T("Ͷ�� %d ��AcceptEx�������"),MAX_POST_ACCEPT );
	else
		this->_ShowMessage(_T("Make AcceptEx completed."), MAX_POST_ACCEPT);

	return true;
}

////////////////////////////////////////////////////////////
//	����ͷŵ�������Դ
void CIOCPModel::_DeInitialize()
{
	// �ر�ϵͳ�˳��¼����
	RELEASE_HANDLE(m_hShutdownEvent);

	// �ͷŹ������߳̾��ָ��
	for( int i=0;i<m_nThreads;i++ )
	{
		RELEASE_HANDLE(m_phWorkerThreads[i]);
	}
	
	RELEASE(m_phWorkerThreads);

	// �ر�IOCP���
	RELEASE_HANDLE(m_hIOCompletionPort);

	// �رռ���Socket
	RELEASE(m_pListenContext);

	if (m_bChineseLang)
		this->_ShowMessage("�ͷ���Դ���.\n");
	else
		this->_ShowMessage("Free Memory.\n");
}


//====================================================================================
//
//				    Ͷ����ɶ˿�����
//
//====================================================================================


//////////////////////////////////////////////////////////////////
// Ͷ��Accept����
bool CIOCPModel::_PostAccept( PER_IO_CONTEXT* pAcceptIoContext )
{
	ASSERT( INVALID_SOCKET!=m_pListenContext->m_Socket );

	// ׼������
	DWORD dwBytes = 0;  
	pAcceptIoContext->m_OpType = ACCEPT_POSTED;  
	WSABUF *p_wbuf   = &pAcceptIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pAcceptIoContext->m_Overlapped;
	
	// Ϊ�Ժ�������Ŀͻ�����׼����Socket( ������봫ͳaccept�������� ) 
	pAcceptIoContext->m_sockAccept  = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);  
	if( INVALID_SOCKET==pAcceptIoContext->m_sockAccept )  
	{
		if (m_bChineseLang)
			this->_ShowMessage("��������Accept��Socketʧ�ܣ��������: %d", WSAGetLastError());
		else
			this->_ShowMessage("Create Socket Accept failed! ErrorCode: %d", WSAGetLastError());
		return false;  
	} 

	// Ͷ��AcceptEx
	if(FALSE == m_lpfnAcceptEx( m_pListenContext->m_Socket, pAcceptIoContext->m_sockAccept, p_wbuf->buf, p_wbuf->len - ((sizeof(SOCKADDR_IN)+16)*2),   
								sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &dwBytes, p_ol))  
	{  
		if(WSA_IO_PENDING != WSAGetLastError())  
		{
			if (m_bChineseLang)
				this->_ShowMessage("Ͷ�� AcceptEx ����ʧ�ܣ��������: %d", WSAGetLastError());
			else
				this->_ShowMessage("Do AcceptEx failed��ErrorCode: %d", WSAGetLastError());
			return false;  
		}  
	} 

	return true;
}


/********************************************************************
*�������ܣ��������пͻ��˽��봦��
*����˵����
PER_SOCKET_CONTEXT* pSocketContext:	����accept������Ӧ���׽��֣����׽�������Ӧ�����ݽṹ��
PER_IO_CONTEXT* pIoContext:			����accept������Ӧ�����ݽṹ��
DWORD		dwIOSize:				���β�������ʵ�ʴ�����ֽ���
********************************************************************/
bool CIOCPModel::_DoAccpet( PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext )
{
	if (pIoContext->m_nTotalBytes > 0)
	{
		//�ͻ�����ʱ����һ�ν���dwIOSize�ֽ�����
		_DoFirstRecvWithData(pIoContext);
	}
	else
	{
		//�ͻ��˽���ʱ��û�з������ݣ���Ͷ��WSARecv���󣬽�������
		_DoFirstRecvWithoutData(pIoContext);

	}

	// 5. ʹ�����֮�󣬰�Listen Socket���Ǹ�IoContext���ã�Ȼ��׼��Ͷ���µ�AcceptEx
	pIoContext->ResetBuffer();
	return this->_PostAccept( pIoContext ); 	
}


/////////////////////////////////////////////////////////////////
//�������ܣ����н��յ����ݵ����ʱ�򣬽��д���
bool CIOCPModel::_DoRecv( PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext )
{
	//������յ�����
	SOCKADDR_IN* ClientAddr = &pSocketContext->m_ClientAddr;
	//this->_ShowMessage( _T("�յ�  %s:%d ��Ϣ��%s"),inet_ntoa(ClientAddr->sin_addr), ntohs(ClientAddr->sin_port),pIoContext->m_wsaBuf.buf );

	//��������
	pIoContext->m_nSendBytes = 0;
	pIoContext->m_nTotalBytes= pIoContext->m_nTotalBytes;
	pIoContext->m_wsaBuf.len = pIoContext->m_nTotalBytes;
	pIoContext->m_wsaBuf.buf = pIoContext->m_szBuffer;
	return PostWrite( pIoContext );
}

/*************************************************************
*�������ܣ�AcceptEx���տͻ����ӳɹ������տͻ���һ�η��͵����ݣ���Ͷ��WSASend����
*����������
PER_IO_CONTEXT* pIoContext:	���ڼ����׽����ϵĲ���
**************************************************************/
bool CIOCPModel::_DoFirstRecvWithData(PER_IO_CONTEXT* pIoContext)
{
	SOCKADDR_IN* ClientAddr = NULL;
	SOCKADDR_IN* LocalAddr = NULL;  
	int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);  

	//1. ����ȡ������ͻ��˵ĵ�ַ��Ϣ
	this->m_lpfnGetAcceptExSockAddrs(pIoContext->m_wsaBuf.buf, pIoContext->m_wsaBuf.len - ((sizeof(SOCKADDR_IN)+16)*2),  
		sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);  

	// �豸���к�
	char szSerial[40] = { 0 };
	ElectrocarProtocol::getEquipmentID((unsigned char*)pIoContext->m_wsaBuf.buf, szSerial);
	strcpy(pIoContext->m_szSerial, szSerial);

	//��ʾ�ͻ�����Ϣ
	if (m_bChineseLang)
		this->_ShowMessage( _T("�ͻ��� %s:%d ��������. ID�� %s"),inet_ntoa(ClientAddr->sin_addr), ntohs(ClientAddr->sin_port), pIoContext->m_szSerial);
	else
		this->_ShowMessage(_T("Client %s:%d Connect. ID: %s"), inet_ntoa(ClientAddr->sin_addr), ntohs(ClientAddr->sin_port), pIoContext->m_szSerial);

	//2.Ϊ�½�����׽Ӵ���PER_SOCKET_CONTEXT���������׽��ְ󶨵���ɶ˿�
	PER_SOCKET_CONTEXT* pNewSocketContext = new PER_SOCKET_CONTEXT;
	pNewSocketContext->m_Socket           = pIoContext->m_sockAccept;
	memcpy(&(pNewSocketContext->m_ClientAddr), ClientAddr, sizeof(SOCKADDR_IN));

	// ����������ϣ������Socket����ɶ˿ڰ�(��Ҳ��һ���ؼ�����)
	if( false==this->_AssociateWithIOCP( pNewSocketContext ) )
	{
		RELEASE( pNewSocketContext );
		return false;
	}  

	//3. �������������µ�IoContext�����������Socket��Ͷ�ݵ�һ��Recv��������
	PER_IO_CONTEXT* pNewIoContext = pNewSocketContext->GetNewIoContext();
	pNewIoContext->m_OpType       = SEND_POSTED;
	pNewIoContext->m_sockAccept   = pNewSocketContext->m_Socket;
	pNewIoContext->m_nTotalBytes  = pIoContext->m_nTotalBytes;
	pNewIoContext->m_nSendBytes   = 0;
	pNewIoContext->m_wsaBuf.len	  = pIoContext->m_nTotalBytes;
	memcpy(pNewIoContext->m_wsaBuf.buf, pIoContext->m_wsaBuf.buf, pIoContext->m_nTotalBytes);	//�������ݵ�WSASend�����Ĳ���������

	//��ʱ�ǵ�һ�ν������ݳɹ�����������Ͷ��PostWrite����ͻ��˷�������
	if( false==this->PostWrite( pNewIoContext) )
	{
		pNewSocketContext->RemoveContext( pNewIoContext );
		return false;
	}

	//4. ���Ͷ�ݳɹ�����ô�Ͱ������Ч�Ŀͻ�����Ϣ�����뵽ContextList��ȥ(��Ҫͳһ���������ͷ���Դ)
	this->_AddToContextList( pNewSocketContext ); 

	return true;
}

/*************************************************************
*�������ܣ�AcceptEx���տͻ����ӳɹ�����ʱ��δ���յ����ݣ���Ͷ��WSARecv����
*����������
PER_IO_CONTEXT* pIoContext:	���ڼ����׽����ϵĲ���
**************************************************************/
bool CIOCPModel::_DoFirstRecvWithoutData(PER_IO_CONTEXT* pIoContext )
{
	//Ϊ�½�����׽��ִ���PER_SOCKET_CONTEXT�ṹ�����󶨵���ɶ˿�
	PER_SOCKET_CONTEXT* pNewSocketContext = new PER_SOCKET_CONTEXT;
	SOCKADDR_IN ClientAddr;
	int Len = sizeof(ClientAddr);

	getpeername(pIoContext->m_sockAccept, (sockaddr*)&ClientAddr, &Len);

	pNewSocketContext->m_Socket           = pIoContext->m_sockAccept;
	memcpy(&(pNewSocketContext->m_ClientAddr), &ClientAddr, sizeof(SOCKADDR_IN));
	
	//�����׽��ְ󶨵���ɶ˿�
	if( false==this->_AssociateWithIOCP( pNewSocketContext ) )
	{
		RELEASE( pNewSocketContext );
		return false;
	} 

	//Ͷ��WSARecv���󣬽�������
	PER_IO_CONTEXT* pNewIoContext = pNewSocketContext->GetNewIoContext();

	//��ʱ��AcceptExδ���յ��ͻ��˵�һ�η��͵����ݣ������������PostRecv���������Կͻ��˵�����
	if( false==this->PostRecv( pNewIoContext) )
	{
		pNewSocketContext->RemoveContext( pNewIoContext );
		return false;
	}

	//���Ͷ�ݳɹ�����ô�Ͱ������Ч�Ŀͻ�����Ϣ�����뵽ContextList��ȥ(��Ҫͳһ���������ͷ���Դ)
	this->_AddToContextList( pNewSocketContext ); 

	return true;
}


/////////////////////////////////////////////////////
// �����(Socket)�󶨵���ɶ˿���
bool CIOCPModel::_AssociateWithIOCP( PER_SOCKET_CONTEXT *pContext )
{
	// �����ںͿͻ���ͨ�ŵ�SOCKET�󶨵���ɶ˿���
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)pContext->m_Socket, m_hIOCompletionPort, (DWORD)pContext, 0);

	if (NULL == hTemp)
	{
		if (m_bChineseLang)
			this->_ShowMessage(("ִ��CreateIoCompletionPort()���ִ���.������룺%d"),GetLastError());
		else
			this->_ShowMessage(("CreateIoCompletionPort() error.Code��%d"), GetLastError());
		return false;
	}

	return true;
}



//////////////////////////////////////////////////////////////
// ���ͻ��˵������Ϣ�洢��������
void CIOCPModel::_AddToContextList( PER_SOCKET_CONTEXT *pHandleData )
{
	EnterCriticalSection(&m_csContextList);

	m_arrayClientContext.Add(pHandleData);
	
	LeaveCriticalSection(&m_csContextList);
}

////////////////////////////////////////////////////////////////
//	�Ƴ�ĳ���ض���Context
void CIOCPModel::_RemoveContext( PER_SOCKET_CONTEXT *pSocketContext )
{
	EnterCriticalSection(&m_csContextList);

	for( int i=0;i<m_arrayClientContext.GetCount();i++ )
	{
		if( pSocketContext==m_arrayClientContext.GetAt(i) )
		{
			RELEASE( pSocketContext );			
			m_arrayClientContext.RemoveAt(i);			
			break;
		}
	}

	LeaveCriticalSection(&m_csContextList);
}

//////////////////////////////////////////////////////////////
// �������в���socket����Ϣ
PER_SOCKET_CONTEXT* CIOCPModel::_FindContext(PER_SOCKET_CONTEXT *pSocketContext)
{
	PER_SOCKET_CONTEXT* pHandleData = NULL;
	EnterCriticalSection(&m_csContextList);

	for( int i=0;i<m_arrayClientContext.GetCount();i++ )
	{
		if( pSocketContext==m_arrayClientContext.GetAt(i) )
		{
			pHandleData = pSocketContext;
			break;
		}
	}

	LeaveCriticalSection(&m_csContextList);
	return pHandleData;
}

////////////////////////////////////////////////////////////////
// ��տͻ�����Ϣ
void CIOCPModel::_ClearContextList()
{
	EnterCriticalSection(&m_csContextList);

	for( int i=0;i<m_arrayClientContext.GetCount();i++ )
	{
		delete m_arrayClientContext.GetAt(i);
	}

	m_arrayClientContext.RemoveAll();

	LeaveCriticalSection(&m_csContextList);
}



////////////////////////////////////////////////////////////////////
// ��ñ�����IP��ַ
CString CIOCPModel::GetLocalIP()
{
	// ��ñ���������
	char hostname[MAX_PATH] = {0};
	gethostname(hostname,MAX_PATH);                
	struct hostent FAR* lpHostEnt = gethostbyname(hostname);
	if(lpHostEnt == NULL)
	{
		return DEFAULT_IP;
	}

	// ȡ��IP��ַ�б��еĵ�һ��Ϊ���ص�IP(��Ϊһ̨�������ܻ�󶨶��IP)
	LPSTR lpAddr = lpHostEnt->h_addr_list[0];      

	// ��IP��ַת�����ַ�����ʽ
	struct in_addr inAddr;
	memmove(&inAddr,lpAddr,4);
	g_serverIP = inet_ntoa(inAddr);
	m_strIP = CString( g_serverIP.c_str() );

	return m_strIP;
}

///////////////////////////////////////////////////////////////////
// ��ñ����д�����������
int CIOCPModel::_GetNoOfProcessors()
{
	SYSTEM_INFO si;

	GetSystemInfo(&si);

	return si.dwNumberOfProcessors;
}

/////////////////////////////////////////////////////////////////////
// ������������ʾ��ʾ��Ϣ
void CIOCPModel::_ShowMessage(const CString szFormat,...) const
{
	// ���ݴ���Ĳ�����ʽ���ַ���
	va_list   arglist;
	CString   strMessage;

	// ����䳤����
	va_start(arglist, szFormat);
	strMessage.FormatV(szFormat,arglist);
	va_end(arglist);

	// ������������ʾ
	CMainDlg* pMain = (CMainDlg*)m_pMain;
	if( m_pMain!=NULL )
	{
		pMain->AddInformation(strMessage);
		TRACE( strMessage+_T("\n") );
	}
}

/////////////////////////////////////////////////////////////////////
// �жϿͻ���Socket�Ƿ��Ѿ��Ͽ���������һ����Ч��Socket��Ͷ��WSARecv����������쳣
// ʹ�õķ����ǳ��������socket�������ݣ��ж����socket���õķ���ֵ
// ��Ϊ����ͻ��������쳣�Ͽ�(����ͻ��˱������߰ε����ߵ�)��ʱ�򣬷����������޷��յ��ͻ��˶Ͽ���֪ͨ��

bool CIOCPModel::_IsSocketAlive(SOCKET s)
{
	int nByteSent=send(s,"",0,0);
	if (-1 == nByteSent) 
		return false;
	return true;
}

///////////////////////////////////////////////////////////////////
//�������ܣ���ʾ��������ɶ˿��ϵĴ���
bool CIOCPModel::HandleError( PER_SOCKET_CONTEXT *pContext,const DWORD& dwErr )
{
	// ����ǳ�ʱ�ˣ����ټ����Ȱ�  
	if(WAIT_TIMEOUT == dwErr)  
	{  	
		// ȷ�Ͽͻ����Ƿ񻹻���...
		if( !_IsSocketAlive( pContext->m_Socket) )
		{
			if (m_bChineseLang)
				this->_ShowMessage( _T("��⵽�ͻ����쳣�˳���") );
			else
				this->_ShowMessage( _T("Client disconnect��") );
			this->_RemoveContext( pContext );
			return true;
		}
		else
		{
			if (m_bChineseLang)
				this->_ShowMessage( _T("���������ʱ��������...") );
			else
				this->_ShowMessage( _T("Network operation timeout��Retry...") );
			return true;
		}
	}  

	// �����ǿͻ����쳣�˳���
	else if( ERROR_NETNAME_DELETED==dwErr )
	{
		if (m_bChineseLang)
			this->_ShowMessage( _T("��⵽�ͻ��˶Ͽ����ӣ�") );
		else
			this->_ShowMessage(_T("Client disconnect��"));
		this->_RemoveContext( pContext );
		return true;
	}

	else
	{
		if (m_bChineseLang)
			this->_ShowMessage( _T("��ɶ˿ڲ������ִ����߳��˳���������룺%d"),dwErr );
		else
			this->_ShowMessage(_T("Thread exit��ErrorCode��%d"), dwErr);
		return false;
	}
}




