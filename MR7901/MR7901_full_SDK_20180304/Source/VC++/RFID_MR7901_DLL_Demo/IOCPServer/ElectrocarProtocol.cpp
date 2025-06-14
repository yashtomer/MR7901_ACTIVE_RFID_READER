/*
* File: ElectrocarProtocol.cpp
* Note: 电动车项目数据包拆解协议
* Author: zhangxinxiang
* Time: 2017-03-31
*/
#include "stdafx.h"
#include "ElectrocarProtocol.h"
#include <stdlib.h>
#include <time.h>
#include "RFID_MR7901_DLL.h"
#include "./json/cJSON.h"
using namespace std;

#ifdef _DEBUG
#pragma comment(lib,"../Debug/RFID_MR7901_DLL.lib")
#else
#pragma comment(lib,"../Release/RFID_MR7901_DLL.lib")
#endif

TAGMSGDATA_MAP TagMsg_Data;
CRITICAL_SECTION m_csTagMsgData;
HWND ElectrocarProtocol::m_MainDlgHwnd = NULL;


bool ElectrocarProtocol::recv_buffer_parse(PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len)
{
	int nResult = RFIDSVR_SUCCESS;
	char device_msg[250] = { 0 };
	int length = 0;
	int cmd = RFID_ReadCmd((unsigned char*)buffer, len, device_msg, &length);

	//解析JSON得到设备ID
	cJSON *root;
	root = cJSON_Parse(device_msg);
	if (NULL == root) {
		return false;
	}
	cJSON *json_device;
	json_device = cJSON_GetObjectItem(root, "device");
	if (NULL == json_device)
	{
		cJSON_Delete(root);
		return false;
	}
	char szSerial[40] = {0};
	strcpy(szSerial, json_device->valuestring);
	cJSON_Delete(root);

	switch (cmd)
	{
	case 0x0008:
		nResult = equipment_register(szSerial, pIoContext, buffer, len);
		break;
	case 0x0001:
		nResult = equipment_login(szSerial, pIoContext, buffer, len);
		break;
	case 0x0003:
		nResult = equipment_heart(szSerial, pIoContext, buffer, len);
		break;
	case 0x0004:
		nResult = equipment_data_upload(szSerial, pIoContext, buffer, len);
		break;
	default:
		break;
	}

    return nResult == RFIDSVR_SUCCESS;
}

int ElectrocarProtocol::send_to_net_client(PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len)
{
	pIoContext->m_wsaBuf.len = len;
	pIoContext->m_nTotalBytes = len;
	memcpy(pIoContext->m_wsaBuf.buf, buffer, len);
	return 0;
}

int ElectrocarProtocol::equipment_register(char* szSerial, PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len)
{
	char host[256] = { 0 };
	strcpy(host, g_serverIP.c_str());
	int port = g_nPort;

	// 下发负载均衡服务器ip、端口
	char json_msg[250] = { 0 };
	sprintf(json_msg, "{\"IP\":\"%s\",\"PORT\":%d}", host, port);

	unsigned char TcpSendBuf[250] = { 0 };
	int TcpSendLen = RFID_Register((unsigned char*)buffer, len, 0x00, json_msg, TcpSendBuf);
	if (TcpSendLen <= 0)
	{
		return RFIDSVR_ERROR_BASE;
	}

	//平台回应包
	send_to_net_client(pIoContext, (char*)TcpSendBuf, TcpSendLen);

	return RFIDSVR_SUCCESS;
}

int ElectrocarProtocol::equipment_login(char* szSerial, PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len)
{
	unsigned char TcpSendBuf[250] = { 0 };
	int TcpSendLen = RFID_Login((unsigned char*)buffer, len, 0x00, TcpSendBuf);
	if (TcpSendLen <= 0)
	{
		return RFIDSVR_ERROR_BASE;
	}

	//平台回应包
	send_to_net_client(pIoContext, (char*)TcpSendBuf, TcpSendLen);

	return RFIDSVR_SUCCESS;
}

int ElectrocarProtocol::equipment_heart(char* szSerial, PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len)
{
	unsigned char TcpSendBuf[250] = { 0 };
	int TcpSendLen = RFID_Heartbeat((unsigned char*)buffer, len, 0x00, TcpSendBuf);
	if (TcpSendLen <= 0)
	{
		return RFIDSVR_ERROR_BASE;
	}

	//平台回应包
	send_to_net_client(pIoContext, (char*)TcpSendBuf, TcpSendLen);

	return RFIDSVR_SUCCESS;
}

int ElectrocarProtocol::equipment_data_upload(char* szSerial, PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len)
{
	unsigned char TcpRecvBuf[9000] = { 0 };
	memcpy(TcpRecvBuf, buffer, len);

	unsigned char TcpSendBuf[250] = { 0 };
	int TcpSendLen = RFID_DataRecord(TcpRecvBuf, len, TcpSendBuf);
	if (TcpSendLen <= 0)
	{
		return RFIDSVR_ERROR_BASE;
	}

	//平台回应包
	send_to_net_client(pIoContext, (char*)TcpSendBuf, TcpSendLen);

	//标签数据解析
	char json_data[9000] = { 0 };
	int length = 0;
	int size = RFID_TagDataParse(TcpRecvBuf, len, json_data, &length);
	TRACE("=======标签个数：%d=======\n", size);

	label_data_parse(szSerial, json_data);

	return RFIDSVR_SUCCESS;
}


void ElectrocarProtocol::label_data_parse(const char* szSerial, char *Tlv_json_data)
{
	TagInfo tag;
	uint64_t iTagID = 0;
	
	//解析JSON数组
	cJSON *root = cJSON_Parse(Tlv_json_data);
	if (NULL != root) 
	{
		cJSON *item = NULL;
		cJSON *arrayItem = cJSON_GetObjectItem(root, "TAG");//取数组
		if (NULL != arrayItem)
		{
			cJSON *object = arrayItem->child;//子对象
			while (object != NULL)
			{
				item = cJSON_GetObjectItem(object, "device");
				if (item != NULL) {
					strcpy(tag.szSerial, item->valuestring);
				}

				item = cJSON_GetObjectItem(object, "tagid");
				if (item != NULL) {
					strcpy(tag.szTagID, item->valuestring);
				}

				item = cJSON_GetObjectItem(object, "tlvtype");//标签类型
				if (item != NULL) {				
					tag.nTlvType = 0;
					if (strcmp(item->valuestring,"0x8B01") == 0) {
						tag.nTlvType = 1;
					}
					else if (strcmp(item->valuestring,"0x8B02") == 0) {
						tag.nTlvType = 2;
					}
				}

				item = cJSON_GetObjectItem(object, "tagtype");
				if (item != NULL) {
					//TRACE("Tag Type = %s\n", item->valuestring);
				}

				item = cJSON_GetObjectItem(object, "antenna");
				if (item != NULL) {
					tag.ichannelid = item->valueint;
				}

				item = cJSON_GetObjectItem(object, "intensity");
				if (item != NULL) {
					tag.intensity = item->valueint;
				}

				item = cJSON_GetObjectItem(object, "entry");
				if (item != NULL) {
					tag.inout_type = item->valueint;
				}

				item = cJSON_GetObjectItem(object, "staying");
				if (item != NULL) {
					tag.iStay_flag = item->valueint;
				}

				item = cJSON_GetObjectItem(object, "alarm");
				if (item != NULL) {
					//TRACE("Alarm State = %d\n", item->valueint);
				}

				item = cJSON_GetObjectItem(object, "time");
				if (item != NULL) {
					strcpy(tag.szTagTime, item->valuestring);
				}

				// Show Window
				iTagID = HexToDecimal(tag.szTagID);
				EnterCriticalSection(&m_csTagMsgData);
				TAGMSGDATA_MAP::iterator iter = TagMsg_Data.find(iTagID);
				if (iter == TagMsg_Data.end())
					tag.index = TagMsg_Data.size();
				else
					tag.index = iter->second.index;
				TagMsg_Data[iTagID] = tag;
				LeaveCriticalSection(&m_csTagMsgData);

				::PostMessage(m_MainDlgHwnd, WM_RECV_TAGDATA, 0, (LPARAM)iTagID);

				object = object->next;
			}
		}
		cJSON_Delete(root);
	}

	return;
}

void ElectrocarProtocol::getEquipmentID(const unsigned char* buffer, char* szSerial)
{
	if( RFID_CMD_LOGIN == buffer[0] && RFID_CMD_LOGIN_AA == buffer[1] )
	{
		char szDeviceId[40] = { 0 };
		char *ptr = szDeviceId;
		char *pSerial = (char*)buffer + 14;
		for (int i = 0; i < 16; i++)
		{
			sprintf(ptr++, "%c", *pSerial++);
		}
		strcpy(szSerial, szDeviceId);
	}
}
