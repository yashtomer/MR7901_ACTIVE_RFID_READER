package test;

import java.net.Socket;
import java.net.ServerSocket;

public class ServerTest {
	
	private ServerSocket serverSocket;
	private int port = 4600;
    
    public void start() {    	
    	try {
	    	serverSocket = new ServerSocket(port);
	    	System.out.println("服务器启动, 监控端口：" + port);
	        System.out.println("等待客户端连接...");
	    
	    	while(true) {
                //方法会产生阻塞，直到某个Socket连接，返回请求连接的Socket
                Socket clientSocket = serverSocket.accept();
                System.out.println("客户端已连接！连接地址：" + clientSocket.getRemoteSocketAddress());
                
                Thread t = new Thread(new ClientHandler(clientSocket));
                t.start();
	    	}
    	}catch (Exception e) {  
            e.printStackTrace();  
        }   	
    }    
    
    public static void main(String[] args) {
    	
    	//打印jdk的位数
    	String bit = System.getProperty("sun.arch.data.model");
    	System.out.println("JDK位数=" + bit);
    	
    	ServerTest server = new ServerTest();
        server.start();
    }
}