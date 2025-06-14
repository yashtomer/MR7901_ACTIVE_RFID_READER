/*******************************************************************
 * Copyright (c) 2015-2017 Marktrace
 * Web:  http://www.marktrace.com/
 * Company: 深圳市华士精成科技有限公司
 *
 * Communication protocol interface library of MR7901/MR7914
 * File: RFID_MR7901_DLL
 * Author : zhxx
 * Date   : 2018/02/01
 * Version: V1.0
 *
 *******************************************************************/

#ifndef RFID_MR7901_DLL_H
#define RFID_MR7901_DLL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WINDOWS
#define RFID_DLL_API __declspec(dllexport)
#else  
#define RFID_DLL_API	
#endif

/* 错误码定义: */
#define PARAMETER_ERROR     (-101)      // Function parameter error.
#define INVALID_BUFFER      (-102)      // Receive invalid buffer because CRC16 check error.
#define JSON_PARSING_ERROR  (-103)      // Json string parameter parsing failure.


/**
   读取设备发起的命令码, 根据命令码调用相应的函数

   @param recv_buffer 网络接收到设备上报的数据包
   @param recv_len 指明recv_buffer的字节数
   @param json_result (出参)报文头信息结果，格式为JSON串，形如:
    {
      "cmd" : "0x0001",              // 命令码
      "seqno" : "00000000",          // 报文流水号
      "device" : "861694034205896",  // 设备ID
      "version": "3.6"               // 设备软件版本号,没有则为空
    }
   @param result_length (出参)指明json_result 结果长度
   @return 返回设备请求的命令码; 出错返回负数（见错误码定义）
*/
RFID_DLL_API int RFID_ReadCmd(const unsigned char *recv_buffer, const int recv_len, 
                              char *json_result, int *result_length);


/**
   设备向平台发起注册 cmd: 0x0008

   @param recv_buffer 网络接收到设备上报的数据包
   @param recv_len 指明recv_buffer的字节数
   @param respond_cmd 平台回应指令： 0x00--注册成功，0xFF--注册拒绝
   @param json_str_server_addr 平台回应负载服务器的IP与端口（注册成功才生效），格式为JSON串，形如：
    {
      "ip" : "218.17.157.214",       // 负载服务器IP
      "port" : 4600,                 // 负载服务器端口
    }
   @param send_buffer (出参)平台要下发给设备的数据包（长度限制为250字节）
   @return 返回要下发给设备的数据包大小; 出错返回负数（见错误码定义）
*/
RFID_DLL_API int RFID_Register(const unsigned char *recv_buffer, const int recv_len, 
                               const unsigned char respond_cmd, 
                               const char *json_str_server_addr, 
                               unsigned char *send_buffer);


/**
   设备向平台发起登录 cmd: 0x0001

   @param recv_buffer 网络接收到设备上报的数据包
   @param recv_len 指明recv_buffer的字节数
   @param respond_cmd 平台回应指令： 0x00--登录成功，0xFF--登录拒绝
   @param send_buffer (出参)平台要下发给设备的数据包（长度限制为250字节）
   @return 返回要下发给设备的数据包大小; 出错返回负数（见错误码定义）
*/
RFID_DLL_API int RFID_Login(const unsigned char *recv_buffer, const int recv_len, 
                            const unsigned char respond_cmd, 
                            unsigned char *send_buffer);


/**
   设备向平台发起心跳 cmd: 0x0003

   @param recv_buffer 网络接收到设备上报的数据包
   @param recv_len 指明recv_buffer的字节数
   @param respond_cmd 平台回应指令： 0x00--没有操作指示
   @param send_buffer (出参)平台要下发给设备的数据包（长度限制为250字节）
   @return 返回要下发给设备的数据包大小; 出错返回负数（见错误码定义）
*/
RFID_DLL_API int RFID_Heartbeat(const unsigned char *recv_buffer, const int recv_len, 
                                const unsigned char respond_cmd, 
                                unsigned char *send_buffer);


/**
   设备向平台发起数据上报 cmd: 0x0004

   @param recv_buffer 网络接收到设备上报的数据包
   @param recv_len 指明recv_buffer的字节数
   @param send_buffer (出参)平台要下发给设备的数据包（长度限制为250字节）
   @return 返回要下发给设备的数据包大小; 出错返回负数（见错误码定义）
*/
RFID_DLL_API int RFID_DataRecord(const unsigned char *recv_buffer, const int recv_len, 
	                             unsigned char *send_buffer);


/**
   对接收到的标签数据做解析 cmd: 0x0004

   @param recv_buffer 网络接收到设备上报的数据包
   @param recv_len 指明recv_buffer的字节数
   @param json_result (出参)解析后得到的标签数据结果，格式为JSON数组，形如：
    {
	  "TAG":[
	  {
        "device": "861694034205706",      //采集器设备ID
        "tagid": "16100001",              //标签ID
        "tlvtype": "0x8B01",              //TLV类型
        "tagtype": "20",                  //标签类型
        "antenna": 1,                     //天线号，为 1,2,3,4 分别对应东、南、西、北 4个天线
        "intensity": -78,                 //信号强度， -78dBm
		"entry": 0,                       //进出基站状态：1：进基站读取范围， 0：出基站读取范围
		"staying": 0,                     //基站停留标识：1：基站停留，0：不是基站停留
        "alarm": 0,                       //报警状态(1byte), 0-正常
        "time": "2018-02-26 15:20:40"     //标签读取时间
      },
	  {......},{......}                   //此处省略
      ]
	}
	@param result_length (出参)指明json_result 结果长度
	@return 返回标签个数; 出错返回负数（见错误码定义）
*/
RFID_DLL_API int RFID_TagDataParse(const unsigned char *recv_buffer, const int recv_len,
                                   char *json_result, int *result_length);


#ifdef __cplusplus
}
#endif

#endif /* RFID_MR7901_DLL_H */