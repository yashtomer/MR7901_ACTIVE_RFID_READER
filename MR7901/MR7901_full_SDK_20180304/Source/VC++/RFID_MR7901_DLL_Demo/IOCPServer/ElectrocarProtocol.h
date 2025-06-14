/*
* File: ElectrocarProtocol.h
* Note: 电动车项目数据包拆解协议
* Author: zhangxinxiang
* Time: 2017-03-31
*/
#ifndef __ELECTROCAR_PROTOCOL_H__
#define __ELECTROCAR_PROTOCOL_H__

#include <iostream>
#include <map>
#include <vector>
#include <stdint.h>
#include "IOCPModel.h"

#define RFIDSVR_SUCCESS		0
#define RFIDSVR_ERROR_BASE	1
#define RFID_CMD_LOGIN		0x55
#define RFID_CMD_LOGIN_AA	0xAA

// 电子标签结构体
typedef struct
{
	unsigned char Tag[4];	//TLV类型
	unsigned char Length[4];
	unsigned char Value[32];
}STRU_TLV;

// 标签数据
typedef struct _TagInfo
{
	long index;				//索引
	int  nTlvType;			//标签类型：1-电子标签，2-考勤标签
	char szTagID[20];		//标签ID
	char szSerial[40];		//采集器编号
	char szTagTime[64];		//读取时间
    int ichannelid;			//天线号
	signed char intensity;	//灵敏度
	short inout_type;		//进出标志（1-进，0-出）
	short iStay_flag;		//停留标识（当停留标识为 1 时，进出标识无效）
	_TagInfo()
	{
		index = -1;
		nTlvType = 0;
		inout_type = 0;
		iStay_flag = 0;
		memset(szTagID, 0, sizeof(szTagID));
		memset(szSerial, 0, sizeof(szSerial));
		memset(szTagTime, 0, sizeof(szTagTime));
	}
}TagInfo;

/* 读到的标签信息，需要在页面显示 */
typedef std::map<uint64_t, TagInfo>  TAGMSGDATA_MAP;
extern TAGMSGDATA_MAP TagMsg_Data;
extern CRITICAL_SECTION m_csTagMsgData; //互斥


class ElectrocarProtocol
{
public:
    static bool recv_buffer_parse(PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len);
	static int send_to_net_client(PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len);
public:
	// 获取设备序列号
	static void getEquipmentID(const unsigned char* buffer, char* szSerial);
protected:
	// 设备注册
	static int equipment_register(char* szSerial, PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len);
	// 设备登录
	static int equipment_login(char* szSerial, PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len);
	// 设备心跳
	static int equipment_heart(char* szSerial, PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len);
	// 数据上报
	static int equipment_data_upload(char* szSerial, PER_IO_CONTEXT* pIoContext, char* buffer, unsigned int len);

protected:
	// 标签数据分析处理
	static void label_data_parse(const char* szSerial, char *Tlv_json_data);

public:
	static HWND m_MainDlgHwnd;
};


//////////////////////////////////////////////////////////////////////////
static int hex_table[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0,
	0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15
};

// 十六进制转十进制
inline uint64_t HexToDecimal(const char* hex_str)
{
	uint64_t iret = 0;
	char *ptr = (char*)hex_str;
	char ch;
	while (ch = *ptr++)
	{
		iret = (iret << 4) | hex_table[ch];
	}
	return iret;
}

//////////////////////////////////////////////////////////////////////////


#endif