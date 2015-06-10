package org.pillowsky.securehttp;

import java.nio.file.Files;
import java.nio.file.NoSuchFileException;
import java.nio.file.Paths;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

public class Guard {
	private SecretKey desKey;
	private Cipher desEnCipher;
	private Cipher desDeCipher;
	
	public Guard(String desKeyfile) throws Exception {
		try {
			desKey = new SecretKeySpec(Files.readAllBytes(Paths.get(desKeyfile)), "DES");
			System.out.format("Load des key from %s%n", desKeyfile);
		} catch (NoSuchFileException e) {
			desKey = KeyGenerator.getInstance("DES").generateKey();
			Files.write(Paths.get(desKeyfile), desKey.getEncoded());
			System.out.format("Create and save des key file to %s%n", desKeyfile);
		}

		desEnCipher = Cipher.getInstance("DES/ECB/PKCS5Padding");
		desEnCipher.init(Cipher.ENCRYPT_MODE, desKey);

		desDeCipher = Cipher.getInstance("DES/ECB/PKCS5Padding");
		desDeCipher.init(Cipher.DECRYPT_MODE, desKey);
		
		//test();
	}
	
	public byte[] desEncrypt(byte[] plaintext) throws Exception {
		return desEnCipher.doFinal(plaintext);
	}
	
	public byte[] desDecrypt(byte[] ciphertext) throws Exception {
		return desDeCipher.doFinal(ciphertext);
	}
	
	public void test() {
		try{
		    byte[] plaintext = "It is plain text".getBytes();
 
		    System.out.println("Text [Byte Format] :" + plaintext);
		    System.out.println("Text :" + new String(plaintext));
 
		    byte[] ciphertext = desEnCipher.doFinal(plaintext);
 
		    System.out.println("Text Encryted : " + ciphertext);
 
		    byte[] output = desDeCipher.doFinal(ciphertext);
 
		    System.out.println("Text Decryted : " + new String(output));
 
		}catch(Exception e){
			e.printStackTrace();
		}
	}
}
