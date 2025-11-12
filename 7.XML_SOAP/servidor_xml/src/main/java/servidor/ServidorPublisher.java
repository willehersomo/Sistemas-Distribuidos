package servidor; 

import javax.xml.ws.Endpoint;


public class ServidorPublisher {

    public static void main(String[] args) {
        String url = "http://127.0.0.1:8080/soap";

        Endpoint.publish(url, new BestiarioServico());

        System.out.println("Servidor rodando!");
        System.out.println("Endpoint: " + url);
        System.out.println("WSDL (Contrato): " + url + "?wsdl");
    }
}