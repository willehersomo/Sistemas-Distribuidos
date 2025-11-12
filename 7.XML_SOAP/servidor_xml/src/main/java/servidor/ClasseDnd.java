package servidor; 

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlType;

@XmlAccessorType(XmlAccessType.FIELD) 
@XmlType(name = "TipoClasseDnd", namespace = "http://bestiario.rpg.com/", propOrder = {
    "nome",
    "dadoVida",
    "atributoPrimario",
    "foco"
})

public class ClasseDnd {

    @XmlElement(name = "Nome", namespace = "http://bestiario.rpg.com/", required = true)
    private String nome;
    
    @XmlElement(name = "DadoVida", namespace = "http://bestiario.rpg.com/", required = true)
    private String dadoVida;
    
    @XmlElement(name = "AtributoPrimario", namespace = "http://bestiario.rpg.com/", required = true)
    private String atributoPrimario;
    
    @XmlElement(name = "Foco", namespace = "http://bestiario.rpg.com/", required = true)
    private String foco;

    public ClasseDnd() { }

    public ClasseDnd(String nome, String dadoVida, String atributoPrimario, String foco) {
        this.nome = nome;
        this.dadoVida = dadoVida;
        this.atributoPrimario = atributoPrimario;
        this.foco = foco;
    }


    public String getNome() { return nome; }
    public void setNome(String nome) { this.nome = nome; }

    public String getDadoVida() { return dadoVida; }
    public void setDadoVida(String dadoVida) { this.dadoVida = dadoVida; }

    public String getAtributoPrimario() { return atributoPrimario; }
    public void setAtributoPrimario(String atributoPrimario) { this.atributoPrimario = atributoPrimario; }

    public String getFoco() { return foco; }
    public void setFoco(String foco) { this.foco = foco; }
}