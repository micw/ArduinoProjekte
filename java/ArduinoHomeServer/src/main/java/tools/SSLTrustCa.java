package tools;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.GeneralSecurityException;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;

import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManagerFactory;

public class SSLTrustCa {
	public static void trustLetsEncryptRootCa() {
		trustCa(SSLTrustCa.class.getResource("ca/DSTRootCAX3.der"));
//		trustCa(SSLTrustCa.class.getResource("ca/isrgrootx1.der"));
//		trustCa(SSLTrustCa.class.getResource("ca/lets-encrypt-x1-cross-signed.der"));
	}

	protected static KeyStore keyStore;

	public synchronized static KeyStore initialize(boolean loadJavaDefaultKeytore)
			throws GeneralSecurityException, IOException {

		if (SSLTrustCa.keyStore == null) {
			KeyStore keyStore = KeyStore.getInstance(KeyStore.getDefaultType());

			if (loadJavaDefaultKeytore) {
				// Load the java default keystore
				Path ksPath = Paths.get(System.getProperty("java.home"), "lib", "security", "cacerts");
				try (InputStream in = Files.newInputStream(ksPath)) {
					keyStore.load(in, "changeit".toCharArray());
				}
			}
			
			SSLTrustCa.keyStore = keyStore;
		}

		return SSLTrustCa.keyStore;
	}

	public synchronized static void trustCa(URL caFile) {
		try {
			Certificate crt;
			CertificateFactory cf = CertificateFactory.getInstance("X.509");
			try (InputStream caInput = new BufferedInputStream(caFile.openStream())) {
				crt = cf.generateCertificate(caInput);
			}

			String certName=((X509Certificate) crt).getSubjectDN().getName();
			KeyStore keyStore = initialize(true);
			keyStore.setCertificateEntry(certName, crt);

			// Set this as the default keystore
			TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
			tmf.init(keyStore);
			SSLContext sslContext = SSLContext.getInstance("TLS");
			sslContext.init(null, tmf.getTrustManagers(), null);
			SSLContext.setDefault(sslContext);
			
			try {
				Class<?> okHtpClient=Class.forName("com.squareup.okhttp.OkHttpClient");
				Field field=okHtpClient.getDeclaredField("defaultSslSocketFactory");
				field.setAccessible(true);
				field.set(null, sslContext.getSocketFactory());
				
			} catch (Exception ex) {
				// ignored
			}
			 
			
			
		} catch (Exception ex) {
			throw new RuntimeException(ex);
		}
	}
}
