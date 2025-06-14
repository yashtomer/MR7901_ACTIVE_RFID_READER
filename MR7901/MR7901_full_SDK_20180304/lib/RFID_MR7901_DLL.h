/*******************************************************************
 * Copyright (c) 2015-2017 Marktrace
 * Web:  http://www.marktrace.com/
 * Company:SHENZHEN MARKTRACE CO.,LTD	
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

/* define of error codes: */
#define PARAMETER_ERROR     (-101)      // Function parameter error.
#define INVALID_BUFFER      (-102)      // Receive invalid buffer because CRC16 check error.
#define JSON_PARSING_ERROR  (-103)      // Json string parameter parsing failure.


/**
   Reader the command codes which from device, accourding to the command codes to call related function

   @param recv_buffer  Lan received the data package from device 
   @param recv_len indicate the bytes of recv_buffer
   @param json_result (out parameter)the result of message header, format in JSON£¬example :
    {
      "cmd" : "0x0001",              //  Command code
      "seqno" : "00000000",          //  message serial number
      "device" : "861694034205896",  // device ID
      "version": "3.6"               //  software version of device, if do not have will be null
    }
   @param result_length (output parameter)indicate result length of json_result 
   @return Return the command code requested by the device; error returns negative (see error code definition)
*/
RFID_DLL_API int RFID_ReadCmd(const unsigned char *recv_buffer, const int recv_len, 
                              char *json_result, int *result_length);


/**
   The device initiates registration to the platform cmd: 0x0008

   @param recv_buffer The network receives the data package reported by the device
   @param recv_len indicate the bytes of recv_buffer
   @param respond_cmd Platform response instructions£º 0x00--registration success£¬0xFF--Registration refused
   @param json_str_server_addr The platform responds to the IP and port of the load server (the registration succeeds only), the format is JSON string, such as:
    {
      "ip" : "218.17.157.214",       //  Load server IP
      "port" : 4600,                 //  Load server port
    }
   @param send_buffer (output parameter)Packets to be sent to the device by the platform (limited to 250 bytes)
   @return Returns the size of the packet to be delivered to the device; error returns negative (see error code definition)
*/
RFID_DLL_API int RFID_Register(const unsigned char *recv_buffer, const int recv_len, 
                               const unsigned char respond_cmd, 
                               const char *json_str_server_addr, 
                               unsigned char *send_buffer);


/**
   The device initiates login to the platform cmd: 0x0001

   @param recv_buffer The network receives the data package reported by the device
   @param recv_len Indicates the number of bytes in recv_buffer
   @param respond_cmd Platform response instructions£º 0x00--login successful£¬0xFF--Login refused
   @param send_buffer (output parameter)Packets to be sent to the device by the platform (limited to 250 bytes)
   @return Returns the size of the packet to be delivered to the device; error returns negative (see error code definition)
*/
RFID_DLL_API int RFID_Login(const unsigned char *recv_buffer, const int recv_len, 
                            const unsigned char respond_cmd, 
                            unsigned char *send_buffer);


/**
   The device initiates a heartbeat to the platform cmd: 0x0003

   @param recv_buffer The network receives the data package reported by the device
   @param recv_len Indicates the number of bytes in recv_buffer
   @param respond_cmd Platform response instructions£º 0x00--No operation instructions
   @param send_buffer (output parameter)Packets to be sent to the device by the platform (limited to 250 bytes)
   @return Returns the size of the packet to be delivered to the device; error returns negative (see error code definition)
*/
RFID_DLL_API int RFID_Heartbeat(const unsigned char *recv_buffer, const int recv_len, 
                                const unsigned char respond_cmd, 
                                unsigned char *send_buffer);


/**
   The device initiates data reporting to the platform cmd: 0x0004

   @param recv_buffer The network receives the data package reported by the device
   @param recv_len Indicates the number of bytes in recv_buffer
   @param send_buffer (output parameter)Packets to be sent to the device by the platform (limited to 250 bytes)
   @return Returns the size of the packet to be delivered to the device; error returns negative (see error code definition)
*/
RFID_DLL_API int RFID_DataRecord(const unsigned char *recv_buffer, const int recv_len, 
	                             unsigned char *send_buffer);


/**
   Parse the received tag data cmd: 0x0004

   @param recv_buffer The network receives the data package reported by the device
   @param recv_len Indicates the number of bytes in recv_buffer
   @param json_result (output parameter)tag data result after analysis, The format is a JSON array,suce as:
    {
	  "TAG":[
	  {
        "device": "861694034205706",      //device ID
        "tagid": "16100001",              //tag ID
        "tlvtype": "0x8B01",              //TLV type
        "tagtype": "20",                  //tag type
        "antenna": 1,                     //Antenna number, the 1, 2, 3, 4 correspond to the East, South, West, North 4 antenna
        "intensity": -78,                 //Signal strength£¬ -78dBm
		"entry": 0,                       //status of entry/exit of basestation£º1£ºentry basestation identify area£¬ 0£ºexit basestation identify area
		"staying": 0,                     //Base station stay identification£º1£ºstaying£¬0£ºnot staying
        "alarm": 0,                       //Alarm status(1byte), 0-normal
        "time": "2018-02-26 15:20:40"     //tag read time
      },
	  {......},{......}                   //Omitted here
      ]
	}
	@param result_length (output parameter)Indicates the length of the json_result result
	@return Return the number of tags; error returns negative (see error code definition)
*/
RFID_DLL_API int RFID_TagDataParse(const unsigned char *recv_buffer, const int recv_len,
                                   char *json_result, int *result_length);


#ifdef __cplusplus
}
#endif

#endif /* RFID_MR7901_DLL_H */