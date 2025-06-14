package test.x64;

//与主机位数不符的dll会被无视！！！
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;  
import com.sun.jna.ptr.IntByReference; 

public interface TestJNA extends Library {
	
	// 获取命令码接口函数
	public int RFID_ReadCmd(byte[] recv_buffer, int recv_len, Pointer json_result, IntByReference resultLength);
	// 设备注册接口函数
	public int RFID_Register(byte[] recv_buffer, int recv_len, byte respond_cmd, String json_str_server_addr, byte[] send_buffer);
	// 设备登录接口函数
	public int RFID_Login(byte[] recv_buffer, int recv_len, byte respond_cmd, byte[] send_buffer);
	// 设备心跳接口函数
	public int RFID_Heartbeat(byte[] recv_buffer, int recv_len, byte respond_cmd, byte[] send_buffer);
	// 设备数据上报接口函数
	public int RFID_DataRecord(byte[] recv_buffer, int recv_len, byte[] send_buffer);
	// 解析设备上报的标签数据
	public int RFID_TagDataParse(byte[] recv_buffer, int recv_len, Pointer json_result, IntByReference resultLength);
	
	// 加载dll
	TestJNA INSTANCE = (TestJNA) Native.loadLibrary("RFID_MR7901_DLL_x64", TestJNA.class);
	
}