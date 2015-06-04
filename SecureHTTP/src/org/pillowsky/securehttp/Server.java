package org.pillowsky.securehttp;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;

public class Server implements Runnable {
	private String localAddr;
	private int localPort;
	private String remoteAddr;
	private int remotePort;
    private ServerSocket serverSocket;
    private ExecutorService pool;

    public Server(String localAddr, int localPort, String remoteAddr, int remotePort) throws IOException {
    	this.localAddr = localAddr;
        this.localPort = localPort;
        this.remoteAddr = remoteAddr;
        this.remotePort = remotePort;
        this.serverSocket = new ServerSocket(localPort, 50, InetAddress.getByName(localAddr));
        this.pool = Executors.newCachedThreadPool();
    }

    @Override
    public void run() {
        while (true) {
            try {
                Socket socket = serverSocket.accept();
                pool.execute(new RequestHandler(socket));
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public class RequestHandler implements Runnable {
        private Socket socket;
        
        RequestHandler(Socket socket) {
            this.socket = socket;
        }

        @Override
        public void run() {
            try {
                System.out.println("Received a connection");

                BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                PrintWriter out = new PrintWriter(socket.getOutputStream());

                String line = in.readLine();
                String[] words = line.split(" ");

                String response;
                if ("GET".equals(words[0]) && "/".equals(words[1])) {
                	response = local();
                } else {
                	if ("POST".equals(words[0]) && "/remote".equals(words[1])) {
                		response = remote();
                	} else {
                		response = invalid();
                	}
                }
                
                out.println("HTTP/1.1 200 OK");
                out.println("Server: SecureHTTP");
                out.println("Connection: close");
                out.println("Content-Type: text/html; charset=UTF-8");
                out.println(String.format("Content-Length: %d", response.length()));
                out.println();

                out.println(response);
                out.println();
                out.flush();

                in.close();
                out.close();
                socket.close();

                System.out.println("Connection closed");
            } catch(IOException e) {
                e.printStackTrace();
            }
        }
        
        private String local() {
        	return String.format("<form action='/remote' method='post'><p>Local Page on %s:%d</p><input type='submit' value='Visit Remote Page' /></form>", localAddr, localPort);
        }
        
        private String remote() {
        	return String.format("<form action='/' method='get'><p>Remote Page on %s:%d</p><input type='submit' value='Visit Local Page' /></form>", localAddr, localPort);
        }
        
        private String invalid() {
        	return "INVALID";
        }
    }
}
