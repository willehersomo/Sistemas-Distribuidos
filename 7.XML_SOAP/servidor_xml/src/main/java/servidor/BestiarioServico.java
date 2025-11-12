package servidor;

import javax.jws.WebMethod;
import javax.jws.WebParam;
import javax.jws.WebService;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;


@WebService(serviceName = "BestiarioServico", targetNamespace = "http://bestiario.rpg.com/")
public class BestiarioServico {

    private List<ClasseDnd> obter_banco_de_dados_classes() {
        List<ClasseDnd> banco = new ArrayList<>();
        banco.add(new ClasseDnd("Bárbaro", "d12", "Força", "Combate Marcial"));
        banco.add(new ClasseDnd("Mago", "d6", "Inteligência", "Magia Arcana"));
        banco.add(new ClasseDnd("Clérigo", "d8", "Sabedoria", "Magia Divina"));
        banco.add(new ClasseDnd("Ladino", "d8", "Destreza", "Perícia"));
        banco.add(new ClasseDnd("Guerreiro", "d10", "Força", "Combate Marcial"));
        banco.add(new ClasseDnd("Feiticeiro", "d6", "Carisma", "Magia Arcana"));
        return banco;
    }

    @WebMethod(operationName = "ListarTodasAsClasses")
    public List<ClasseDnd> listarTodasAsClasses() {
        return obter_banco_de_dados_classes();
    }


    @WebMethod(operationName = "PesquisarClasses")
    public List<ClasseDnd> pesquisarClasses(
            @WebParam(name = "Nome") String nome,
            @WebParam(name = "AtributoPrimario") String atributo) 
            throws FalhaLogicaException 
    {
        
        if ((nome == null || nome.trim().isEmpty()) && (atributo == null || atributo.trim().isEmpty())) {
            throw new FalhaLogicaException("Cliente.NenhumCriterio: Pelo menos um critério (Nome ou AtributoPrimario) deve ser fornecido.");
        }

        List<ClasseDnd> bancoDados = obter_banco_de_dados_classes();
        
        final String nomeLower = (nome != null) ? nome.toLowerCase() : "";
        final String attrLower = (atributo != null) ? atributo.toLowerCase() : "";
        final boolean nomeVazio = nomeLower.isEmpty();
        final boolean attrVazio = attrLower.isEmpty();

        List<ClasseDnd> resultados = bancoDados.stream()
            .filter(c -> {
                boolean matchNome = nomeVazio || c.getNome().toLowerCase().contains(nomeLower);
                boolean matchAttr = attrVazio || c.getAtributoPrimario().toLowerCase().contains(attrLower);
                return matchNome && matchAttr;
            })
            .collect(Collectors.toList());

        if (resultados.isEmpty()) {
            String msg = String.format("Cliente.NenhumResultado: Nenhum resultado encontrado para Nome='%s' E AtributoPrimario='%s'", nome, atributo);
            throw new FalhaLogicaException(msg);
        }

        return resultados;
    }
}