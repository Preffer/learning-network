package org.pillowsky.securehttp;

import org.pillowsky.securehttp.Peer;

public class SecureHTTP {
	public static void main(String[] args) {
        if (args.length != 5) {
	        System.out.println("Usage: java SecureHTTP <local address> <local port> <remote address> <remote port> <des keyfile>");
	        System.exit(1);
	    }
		
		try {
			String localAddr = args[0];
			int localPort = Integer.parseInt(args[1]);
			String remoteAddr = args[2];
			int remotePort = Integer.parseInt(args[3]);
			String  desKeyfile = args[4];

			Peer server = new Peer(localAddr, localPort, remoteAddr, remotePort, desKeyfile);
		    System.out.format("SecureHTTP peer started. Local: %s:%s, Remote: %s:%s%n", localAddr, localPort, remoteAddr, remotePort);
		    server.run();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
