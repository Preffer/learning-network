package org.pillowsky.securehttp;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintStream;
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

                String response;
                String line = in.readLine();
                String[] words = line.split(" ");
                if ("GET".equals(words[0]) && "/".equals(words[1])) {
                	response = local();
                } else {
                	if ("POST".equals(words[0])) {
                		switch(words[1]) {
	                		case "/remote":
	                			response = remote();
	                			break;
	                		case "/portal":
	                			response = portal();
	                			break;
	                		default:
	                			response = notFound();
                		}
             		} else {
                		response = notFound();
                	}
                }

                out.print(response);
                out.flush();
                out.close();
                socket.close();

                System.out.println("Connection closed");
            } catch(IOException e) {
                e.printStackTrace();
            }
        }

        private String local() {
        	String body = String.format("<form action='/remote' method='post'><p>Local Page on %s:%d</p><input type='submit' value='Visit Remote Page' /></form>", localAddr, localPort);
        	return buildResponse(body, 200, "OK");
        }
        
        private String remote() {
        	try {
        		Socket client = new Socket(remoteAddr, remotePort);

                PrintStream out = new PrintStream(client.getOutputStream());
                BufferedReader in = new BufferedReader(new InputStreamReader(client.getInputStream()));

                out.println("POST /portal HTTP/1.1");
                out.println();

                StringBuilder body = new StringBuilder();
                String line;
                while ((line = in.readLine()) != null) {
                	body.append(line);
                	body.append("\n");
                }
                body.append("\n\n");

                out.flush();
                out.close();
                client.close();
            	return body.toString();
        	} catch(IOException e) {
                e.printStackTrace();
                return e.toString();
            }
        }
        
        private String portal() {
        	String body = String.format("<form action='/'><p>Remote Page on %s:%d</p><input type='submit' value='Visit Local Page' /></form>", localAddr, localPort);
        	return buildResponse(body, 200, "OK");
        }

        private String notFound() {
        	return buildResponse("404 Not Found", 404, "Not Found");
        }
        
        private String buildResponse(String body, int status, String textStatus) {
        	StringBuilder response = new StringBuilder();
        	response.append(String.format("HTTP/1.1 %d %s%n", status, textStatus));
        	response.append("Server: SecureHTTP\n");
        	response.append("Connection: close\n");
        	response.append("Content-Type: text/html; charset=UTF-8\n");
        	response.append(String.format("Content-Length: %d%n%n", body.length()));
        	response.append(body);
        	response.append("\n\n");
        	return response.toString();
        }
    }
}
