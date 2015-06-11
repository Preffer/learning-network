package org.pillowsky.securehttp;

import java.nio.file.Files;
import java.nio.file.NoSuchFileException;
import java.nio.file.Paths;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Signature;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

public class Guard {
	private SecretKey desKey;
	private PublicKey rsaPublicKey;
	private PrivateKey rsaPrivateKey;
	private Cipher desEnCipher;
	private Cipher desDeCipher;
	private Signature rsaSigner;
	private Signature rsaVerifier;

	public Guard(String desKeyfile, String privateKeyfile, String publicKeyfile) throws Exception {
		try {
			desKey = new SecretKeySpec(Files.readAllBytes(Paths.get(desKeyfile)), "DES");
			System.out.format("Load des key from %s%n", desKeyfile);
		} catch (NoSuchFileException e) {
			desKey = KeyGenerator.getInstance("DES").generateKey();
			Files.write(Paths.get(desKeyfile), desKey.getEncoded());
			System.out.format("Create and save des key to %s%n", desKeyfile);
		}

		try {
			KeyFactory factory = KeyFactory.getInstance("RSA");
			rsaPrivateKey = factory.generatePrivate(new PKCS8EncodedKeySpec(Files.readAllBytes(Paths.get(privateKeyfile))));
			rsaPublicKey = factory.generatePublic(new X509EncodedKeySpec(Files.readAllBytes(Paths.get(publicKeyfile))));
			System.out.format("Load rsa private key from %s%n", privateKeyfile);
			System.out.format("Load rsa public key from %s%n", publicKeyfile);
		} catch (NoSuchFileException e) {
			KeyPair pair = KeyPairGenerator.getInstance("RSA").generateKeyPair();
			rsaPrivateKey = pair.getPrivate();
			rsaPublicKey = pair.getPublic();
			Files.write(Paths.get(privateKeyfile), new PKCS8EncodedKeySpec(rsaPrivateKey.getEncoded()).getEncoded());
			Files.write(Paths.get(publicKeyfile), new X509EncodedKeySpec(rsaPublicKey.getEncoded()).getEncoded());
			System.out.format("Create and save rsa private key to %s%n", privateKeyfile);
			System.out.format("Create and save rsa public key to %s%n", publicKeyfile);
			System.out.println("Distribute rsa keyfile and run again.");
			System.exit(1);
		}

		desEnCipher = Cipher.getInstance("DES/ECB/PKCS5Padding");
		desEnCipher.init(Cipher.ENCRYPT_MODE, desKey);

		desDeCipher = Cipher.getInstance("DES/ECB/PKCS5Padding");
		desDeCipher.init(Cipher.DECRYPT_MODE, desKey);

		rsaSigner = Signature.getInstance("SHA1withRSA");
		rsaSigner.initSign(rsaPrivateKey);

		rsaVerifier = Signature.getInstance("SHA1withRSA");
		rsaVerifier.initVerify(rsaPublicKey);
	}

	public byte[] desEncrypt(byte[] plaintext) throws Exception {
		return desEnCipher.doFinal(plaintext);
	}

	public byte[] desDecrypt(byte[] ciphertext) throws Exception {
		return desDeCipher.doFinal(ciphertext);
	}

	public byte[] rsaSign(byte[] data) throws Exception {
		rsaSigner.update(data);
		return rsaSigner.sign();
	}

	public boolean rsaVerify(byte[] data, byte[] sign) throws Exception {
		rsaVerifier.update(data);
		return rsaVerifier.verify(sign);
	}
}
