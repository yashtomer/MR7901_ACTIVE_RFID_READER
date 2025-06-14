package test;

import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import java.net.Socket;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import test.x64.TestJNA;// 64λjdk����64λdll

public class ClientHandler implements Runnable {
	private InputStream isReader = null;
	private OutputStream osWrite = null;
	private Socket socket = null;
	private byte[] recv_buff = new byte[10240];
	private int recv_len = 0;

	public ClientHandler(Socket clientSocket) {
		try {
			socket = clientSocket;
			isReader = socket.getInputStream();
			osWrite = socket.getOutputStream();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void run() {
		String message;
		try {
			while (true) {
				recv_len = isReader.read(recv_buff);
				message = BytesHexString(recv_buff, recv_len);
				System.out.println(message); // ��ӡ���յ����ֽ���
				
				int cmd = GetCmd(recv_buff, recv_len);
            	switch (cmd) {
            	case 0x0008:
            		Register(recv_buff, recv_len);
            		break;
            	case 0x0001:
            		Login(recv_buff, recv_len);
            		break;
            	case 0x0003:
            		Heartbeat(recv_buff, recv_len);
            		break;
            	case 0x0004:
            		{
	            		DataRecord(recv_buff, recv_len);
	            		TagDataParse(recv_buff, recv_len);            		
            		}
            		break;
            	default:
            		break;
            	}
			}
		} catch (Exception e) {
			System.out.println("Client socket is closed! ");	
		}
	}
	
	public static String BytesHexString(byte[] b, int length) {
        String ret = "";
        for (int i = 0; i < length; i++) {
            String hex = Integer.toHexString(b[i] & 0xFF);
            if (hex.length() == 1) {
            	hex = '0' + hex;
            }
            ret += hex.toUpperCase();
        }
        return ret;
    }
	
	/**
     * ��ȡ�豸����������
     */
    public int GetCmd(byte[] recv_buff, int recv_len) {
    	try {
    		Pointer result_msg = new Memory(250);
            IntByReference result_length = new IntByReference(); //int* resultlength
        	int cmd = TestJNA.INSTANCE.RFID_ReadCmd(recv_buff, recv_len, result_msg, result_length);
        	
        	//��ӡ��ͷJSON�ַ���
        	int len = result_length.getValue();
        	String jsonString = new String(result_msg.getByteArray(0, len));
        	System.out.println("command = 000" + cmd + ", result json string = " + jsonString);
        	
        	return cmd;
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    	
    	return 0;
    }
	
	/**
     * �豸ע��
     */
    public void Register(byte[] recv_buff, int recv_len) {
    	try {
    		byte respond_cmd = 0x00;
    		byte[] send_buffer = new byte[250];	
    		String server_addr = "{\"IP\":\"127.0.0.1\",\"PORT\":4600}";
    		int send_len = TestJNA.INSTANCE.RFID_Register(recv_buff, recv_len, respond_cmd, server_addr, send_buffer);
    		System.out.println("�豸ע��, ƽ̨�������ݰ���С = " + send_len);
    		
    		// ��ͻ��˻ظ���Ϣ
            osWrite.write(send_buffer, 0, send_len);
            osWrite.flush();

    	} catch (Exception e) {
    		e.printStackTrace();
    	}    	
    }
    
    /**
     * �豸��¼
     */
    public void Login(byte[] recv_buff, int recv_len) {
    	try {
    		byte respond_cmd = 0x00;
    		byte[] send_buffer = new byte[250];
    		int send_len = TestJNA.INSTANCE.RFID_Login(recv_buff, recv_len, respond_cmd, send_buffer);
    		System.out.println("�豸��¼, ƽ̨�������ݰ���С = " + send_len);
    		
    		// ��ͻ��˻ظ���Ϣ
            osWrite.write(send_buffer, 0, send_len);
            osWrite.flush();
            
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    }
    
    /**
     * �豸����
     */
    public void Heartbeat(byte[] recv_buff, int recv_len) {
    	try {
    		byte respond_cmd = 0x00;
    		byte[] send_buffer = new byte[250];	        				
    		int send_len = TestJNA.INSTANCE.RFID_Heartbeat(recv_buff, recv_len, respond_cmd, send_buffer);
    		System.out.println("�豸����, ƽ̨�������ݰ���С = " + send_len);
    		
    		// ��ͻ��˻ظ���Ϣ               
            osWrite.write(send_buffer, 0, send_len);    
            osWrite.flush();
            
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    }

    /**
     * �����ϱ�
     */
    public void DataRecord(byte[] recv_buff, int recv_len) {
    	try {
    		byte[] send_buffer = new byte[250];	        				
    		int send_len = TestJNA.INSTANCE.RFID_DataRecord(recv_buff, recv_len, send_buffer);
    		System.out.println("�����ϱ�, ƽ̨�������ݰ���С = " + send_len);
    		
    		// ��ͻ��˻ظ���Ϣ               
            osWrite.write(send_buffer, 0, send_len);    
            osWrite.flush();
            
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    }
    
    /**
     * ��ǩ���ݽ���
     */
    public void TagDataParse(byte[] recv_buff, int recv_len) {
    	try {
    		Pointer result_msg = new Memory(9000);
            IntByReference result_length = new IntByReference();
    		int count = TestJNA.INSTANCE.RFID_TagDataParse(recv_buff, recv_len, result_msg, result_length);
    		
    		int len = result_length.getValue();
    		String jsonString = new String(result_msg.getByteArray(0, len));
    		
    		System.out.println("================��ǩ���ݽ�����================");    		
    		System.out.println("resultLength = " + len + ", result = " + jsonString);
    		System.out.println("================��ǩ������"+ count + "================");
    		
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    }
}