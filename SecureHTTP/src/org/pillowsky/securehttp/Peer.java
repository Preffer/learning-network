package org.pillowsky.securehttp;

import java.io.IOException;

import org.pillowsky.securehttp.Server;

public class Peer {
	public static void main(String[] args) {
        if (args.length != 4) {
	        System.out.println("Usage: java Peer <local address> <local port> <remote address> <remote port>");
	        System.exit(1);
	    }

		try {
			String localAddr = args[0];
			int localPort = Integer.parseInt(args[1]);
			String remoteAddr = args[2];
			int remotePort = Integer.parseInt(args[3]);

			Server server = new Server(localAddr, localPort, remoteAddr, remotePort);
		    System.out.format("SecureHTTP peer started. Local: %s:%s, Remote: %s:%s%n", localAddr, localPort, remoteAddr, remotePort);
		    server.run();
		} catch (NumberFormatException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
