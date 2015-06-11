package org.pillowsky.securehttp;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.ServerSocket;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.Date;
import java.util.Arrays;
import org.pillowsky.securehttp.Guard;

public class Peer implements Runnable {
	private String localAddr;
	private int localPort;
	private String remoteAddr;
	private int remotePort;
	private ServerSocket serverSocket;
	private ExecutorService pool;
	private DateFormat logFormat;
	private Guard guard;

	public Peer(String localAddr, int localPort, String remoteAddr, int remotePort, String desKeyfile, String privateKeyfile, String publicKeyfile) throws Exception {
		this.localAddr = localAddr;
		this.localPort = localPort;
		this.remoteAddr = remoteAddr;
		this.remotePort = remotePort;
		this.serverSocket = new ServerSocket(localPort, 50, InetAddress.getByName(localAddr));
		this.pool = Executors.newCachedThreadPool();
		this.logFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		this.guard = new Guard(desKeyfile, privateKeyfile, publicKeyfile);
	}

	@Override
	public void run() {
		while (true) {
			try {
				pool.execute(new RequestHandler(serverSocket.accept()));
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}

	public class RequestHandler implements Runnable {
		private Socket clientSocket;
		private String clientAddr;
		private int clientPort;
		private InputStream inStream;
		private OutputStream outStream;
		private BufferedReader inReader;
		private PrintWriter outWriter;

		RequestHandler(Socket socket) {
			this.clientSocket = socket;
			this.clientAddr = socket.getInetAddress().getHostAddress();
			this.clientPort = socket.getPort();
		}

		@Override
		public void run() {
			try {
				inStream = clientSocket.getInputStream();
				outStream = clientSocket.getOutputStream();
				inReader = new BufferedReader(new InputStreamReader(inStream));
				outWriter = new PrintWriter(outStream);

				String line = inReader.readLine();
				String[] words = line.split(" ");
				while (inReader.readLine().length() > 0) {};

				System.out.format("[%s] %s:%d %s%n", logFormat.format(new Date()), clientAddr, clientPort, line);

				switch (words[0]) {
				case "GET":
					switch (words[1]) {
					case "/":
						local();
						break;
					default:
						notFound();
					}
					break;
				case "POST":
					switch (words[1]) {
					case "/remote":
						remote();
						break;
					case "/portal":
						portal();
						break;
					default:
						notFound();
					}
					break;
				default:
					notFound();
				}

				outWriter.flush();
				clientSocket.close();
			} catch(Exception e) {
				e.printStackTrace();
			}
		}

		private void local() {
			StringBuilder body = new StringBuilder();
			body.append("<form action='/remote' method='post'>");
			body.append(String.format("<p>Local Page On %s:%d</p>", localAddr, localPort));
			body.append(String.format("<p>Access From %s:%d</p>", clientAddr, clientPort));
			body.append("<input type='submit' value='Visit Remote Page' />");
			body.append("</form>");
			outWriter.print(buildResponse(body, "200 OK"));
		}

		private void remote() {
			try {
				Socket proxySocket = new Socket(remoteAddr, remotePort);
				OutputStream req = proxySocket.getOutputStream();
				InputStream res = proxySocket.getInputStream();

				req.write("POST /portal HTTP/1.0\n\n".getBytes());

				byte[] buffer = new byte[8192];
				ByteArrayOutputStream body = new ByteArrayOutputStream();

				int bytesRead;
				while ((bytesRead = res.read(buffer)) > 0) {
					body.write(buffer, 0, bytesRead);
				}
				proxySocket.close();

				byte[] packet = body.toByteArray();
				int cipherLength = ByteBuffer.wrap(packet, 0, 4).getInt();
				byte[] encrypted = Arrays.copyOfRange(packet, 4, 4 + cipherLength);
				byte[] sign = Arrays.copyOfRange(packet, 4 + cipherLength, packet.length);

				if (guard.rsaVerify(encrypted, sign)) {
					outStream.write(guard.desDecrypt(encrypted));
				} else {
					outWriter.print(buildResponse("Security Validation Failed", "500 Internal Server Error"));
				}
			} catch(Exception e) {
				e.printStackTrace();
				outWriter.print(buildResponse(e.toString(), "500 Internal Server Error"));
			}
		}

		private void portal() {
			try {
				StringBuilder body = new StringBuilder();
				body.append(String.format("<p>Remote Page On %s:%d</p>", localAddr, localPort));
				body.append(String.format("<p>Access From %s:%d</p>", clientAddr, clientPort));
				body.append("<a href='/'><button>Visit Local Page</button></a>");

				byte[] encrypted = guard.desEncrypt(buildResponse(body, "200 OK").getBytes());
				byte[] sign = guard.rsaSign(encrypted);

				outStream.write(ByteBuffer.allocate(4).putInt(encrypted.length).array());
				outStream.write(encrypted);
				outStream.write(sign);
			} catch(Exception e) {
				e.printStackTrace();
			}
		}

		private void notFound() {
			outWriter.print(buildResponse("404 Not Found", "404 Not Found"));
		}

		private String buildResponse(CharSequence body, String textStatus) {
			StringBuilder response = new StringBuilder();
			response.append(String.format("HTTP/1.0 %s%n", textStatus));
			response.append("Server: SecureHTTP\n");
			response.append("Content-Type: text/html; charset=UTF-8\n");
			response.append(String.format("Content-Length: %d%n%n", body.length()));
			response.append(body);
			response.append("\n\n");
			return response.toString();
		}
	}
}
