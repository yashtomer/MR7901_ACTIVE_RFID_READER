package test;

import java.net.Socket;
import java.net.ServerSocket;

public class ServerTest {
	
	private ServerSocket serverSocket;
	private int port = 4600;
    
    public void start() {    	
    	try {
	    	serverSocket = new ServerSocket(port);
	    	System.out.println("����������, ��ض˿ڣ�" + port);
	        System.out.println("�ȴ��ͻ�������...");
	    
	    	while(true) {
                //���������������ֱ��ĳ��Socket���ӣ������������ӵ�Socket
                Socket clientSocket = serverSocket.accept();
                System.out.println("�ͻ��������ӣ����ӵ�ַ��" + clientSocket.getRemoteSocketAddress());
                
                Thread t = new Thread(new ClientHandler(clientSocket));
                t.start();
	    	}
    	}catch (Exception e) {  
            e.printStackTrace();  
        }   	
    }    
    
    public static void main(String[] args) {
    	
    	//��ӡjdk��λ��
    	String bit = System.getProperty("sun.arch.data.model");
    	System.out.println("JDKλ��=" + bit);
    	
    	ServerTest server = new ServerTest();
        server.start();
    }
}