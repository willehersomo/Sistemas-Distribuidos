package servidor; 

import javax.xml.ws.WebFault;

@WebFault(name = "FalhaLogica", targetNamespace = "http://bestiario.rpg.com/")
public class FalhaLogicaException extends Exception {

    public FalhaLogicaException(String message) {
        super(message);
    }
}