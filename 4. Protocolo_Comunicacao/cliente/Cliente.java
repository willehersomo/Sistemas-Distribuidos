import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.charset.StandardCharsets;
import java.util.Scanner;
import java.util.regex.Pattern;

public class Cliente {
    private static final String HOST = "127.0.0.1";
    private static final int PORTA = 8080;

    private static final byte TIPO_DOC_CPF = 0x01;
    private static final byte TIPO_DOC_CNPJ = 0x02;

    private static final Pattern PADRAO_NAO_DIGITO = Pattern.compile("[^\\d]");

    public static void main(String[] args) {
        try (Scanner leitor_entrada = new Scanner(System.in)) {
            executar_loop_principal(leitor_entrada);
        }
    }

    private static void executar_loop_principal(Scanner leitor_entrada) {
        while (true) {
            imprimir_menu();
            String escolha_usuario = leitor_entrada.nextLine();

            switch (escolha_usuario) {
                case "1":
                    tratar_validacao_documento(leitor_entrada, TIPO_DOC_CPF, "CPF");
                    break;
                case "2":
                    tratar_validacao_documento(leitor_entrada, TIPO_DOC_CNPJ, "CNPJ");
                    break;
                case "0":
                    System.out.println("Saindo...");
                    return;
                default:
                    System.out.println("Opção inválida. Tente novamente.");
                    break;
            }
        }
    }

    private static void imprimir_menu() {
        System.out.println("\nEscolha uma opção:");
        System.out.println("1 - Validar CPF");
        System.out.println("2 - Validar CNPJ");
        System.out.println("0 - Sair");
        System.out.print("Opção: ");
    }

    private static void tratar_validacao_documento(Scanner leitor_entrada, byte tipo_doc, String nome_doc) {
        String doc_limpo = obter_e_limpar_entrada_documento(leitor_entrada, nome_doc);

        if (doc_limpo == null) {
        }

        try {
            int codigo_status = solicitar_validacao_ao_servidor(tipo_doc, doc_limpo);
            imprimir_resultado_validacao(codigo_status);
        } catch (UnknownHostException e) {
            System.err.println("Erro: Host desconhecido. Verifique o endereço do servidor: " + HOST);
        } catch (IOException e) {
            System.err.println("Erro de comunicação com o servidor: " + e.getMessage());
        } catch (Exception e) {
            System.err.println("Ocorreu um erro inesperado: " + e.getMessage());
        }
    }

    private static String obter_e_limpar_entrada_documento(Scanner leitor_entrada, String nome_doc) {
        System.out.printf("Digite o número do %s (somente dígitos ou formatado): ", nome_doc);
        String documento_texto = leitor_entrada.nextLine();
        String doc_limpo = PADRAO_NAO_DIGITO.matcher(documento_texto).replaceAll("");

        if (doc_limpo.isEmpty()) {
            System.err.println("Entrada inválida. O documento não pode estar vazio.");
            return null;
        }
        return doc_limpo;
    }

    private static int solicitar_validacao_ao_servidor(byte tipo_doc, String doc_limpo) throws IOException {
        System.out.println("Conectando ao servidor para validar: " + doc_limpo);

        try (Socket socket = new Socket(HOST, PORTA);
             DataOutputStream saida_dados = new DataOutputStream(socket.getOutputStream());
             DataInputStream entrada_dados = new DataInputStream(socket.getInputStream())) {

            byte[] dados_envio = doc_limpo.getBytes(StandardCharsets.UTF_8);
            short tamanho_dados = (short) dados_envio.length;

            saida_dados.writeByte(tipo_doc);
            saida_dados.writeShort(tamanho_dados);
            saida_dados.write(dados_envio);
            saida_dados.flush();

            return entrada_dados.read();
        }
    }

    private static void imprimir_resultado_validacao(int codigo_status) {
        System.out.println("Resultado (código " + codigo_status + "):");
        switch (codigo_status) {
            case 0x01:
                System.out.println("Válido");
                break;
            case 0x00:
                System.out.println("Inválido");
                break;
            case 0xFF:
                System.out.println("Erro (servidor retornou erro ou requisição malformada)");
                break;
            case -1:
                System.out.println("O servidor encerrou a conexão inesperadamente.");
                break;
            default:
                System.out.println("Desconhecido (código de status não reconhecido: " + codigo_status + ")");
                break;
        }
    }
}