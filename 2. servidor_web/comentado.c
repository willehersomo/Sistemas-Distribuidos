/*
    Servidor Web Simples em C
    -------------------------
    Este código implementa um servidor HTTP básico que responde a requisições GET,
    servindo arquivos HTML de um diretório específico. Utiliza threads para tratar
    múltiplas conexões simultâneas e mutex para proteger variáveis compartilhadas.

    Cada função está comentada detalhadamente para facilitar o entendimento,
    inclusive explicando como funcionam conceitos como sockets, threads, mutex,
    manipulação de arquivos e strings.
*/

#include <stdio.h>              // Funções de entrada/saída (printf, fopen, fread, etc)
#include <stdlib.h>             // Funções utilitárias (malloc, free, exit)
#include <string.h>             // Manipulação de strings (strlen, strcpy, strcat, etc)
#include <time.h>               // Manipulação de data/hora (time, strftime)
#include <sys/socket.h>         // Funções de socket (socket, bind, listen, accept, send, recv)
#include <netinet/in.h>         // Estruturas para endereços de internet (sockaddr_in)
#include <unistd.h>             // Funções POSIX (close, read, write)
#include <pthread.h>            // Threads (pthread_create, pthread_mutex, etc)
#include <arpa/inet.h>          // Conversão de endereços IP (inet_ntoa)

#define PORTA 8080              // Porta onde o servidor irá escutar
#define BUFFER_SIZE 2048        // Tamanho máximo do buffer para mensagens HTTP
#define QTE_SOCKETS 5           // Número máximo de conexões simultâneas permitidas

#define LOCATION "/tmp/www/"    // Diretório onde os arquivos HTML estão armazenados

int new_socket[QTE_SOCKETS];    // Array para armazenar os sockets dos clientes conectados
int qtde_sockets = 0;           // Contador de conexões ativas
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para proteger acesso ao contador de conexões

// Estrutura para armazenar os dados principais de uma requisição HTTP
typedef struct
{
    char metodo[50];    // Método HTTP (ex: GET, POST)
    char caminho[50];   // Caminho do arquivo solicitado (ex: /index.html)
    char protocolo[50]; // Protocolo HTTP (ex: HTTP/1.1)
} requisicao;

/*
    Função: retirar_barra_caminho
    -----------------------------
    Remove a barra inicial '/' do caminho do arquivo, se existir.
    Isso é útil porque o navegador envia caminhos como "/index.html",
    mas no sistema de arquivos queremos "index.html".
*/
void retirar_barra_caminho(char caminho[])
{
    int i;
    int tamanho = strlen(caminho);

    // Se o primeiro caractere for '/', desloca todos para a esquerda
    if (tamanho > 0 && caminho[0] == '/')
    {
        for (i = 0; i < tamanho; i++)
        {
            caminho[i] = caminho[i + 1];
        }
    }
}

/*
    Função: ler_arquivo
    -------------------
    Lê o conteúdo de um arquivo do disco e retorna como uma string.
    - Monta o caminho completo do arquivo usando o diretório base.
    - Abre o arquivo para leitura.
    - Lê todo o conteúdo e retorna um ponteiro para a string alocada.
    - Se não conseguir abrir, retorna NULL.
*/
char *ler_arquivo(char nome_arquivo[])
{
    char caminho_completo[512];
    // Monta o caminho completo do arquivo (ex: /tmp/www/index.html)
    snprintf(caminho_completo, sizeof(caminho_completo), "%s%s", LOCATION, nome_arquivo);

    FILE *arquivo = fopen(caminho_completo, "r"); // Abre arquivo para leitura
    if (arquivo == NULL)
    {
        perror("Erro ao abrir arquivo"); // Mostra erro se não conseguir abrir
        printf("Caminho tentado: %s\n", caminho_completo);
        return NULL;
    }
    else
    {
        // Move o ponteiro para o final para descobrir o tamanho do arquivo
        fseek(arquivo, 0, SEEK_END);
        long tamanho = ftell(arquivo); // Obtém tamanho do arquivo
        fseek(arquivo, 0, SEEK_SET);   // Volta para o início

        // Aloca memória suficiente para o conteúdo do arquivo
        char *conteudo = (char *)malloc(tamanho + 1);
        if (conteudo == NULL)
        {
            fclose(arquivo);
            return NULL;
        }

        // Lê o conteúdo do arquivo para a memória
        fread(conteudo, 1, tamanho, arquivo);
        conteudo[tamanho] = '\0'; // Finaliza a string
        fclose(arquivo); // Fecha o arquivo
        return conteudo; // Retorna o conteúdo lido
    }
}

/*
    Função: pagina_resposta
    -----------------------
    Monta a resposta HTTP completa, incluindo cabeçalhos e o conteúdo do arquivo solicitado.
    - Se o arquivo não existir, retorna a página 404.
    - Adiciona cabeçalhos HTTP como status, data, servidor, tipo de conteúdo, etc.
    - Retorna a resposta pronta para ser enviada ao cliente.
*/
char *pagina_resposta(char nome_arquivo[])
{
    int codigo = 200; // Código de sucesso HTTP
    char *pagina = ler_arquivo(nome_arquivo); // Lê o arquivo solicitado
    if (pagina == NULL)
    {
        pagina = ler_arquivo("404.html"); // Se não encontrar, retorna página 404
        codigo = 404;
    }

    int tamanho = strlen(pagina); // Tamanho do conteúdo HTML
    char tam_char[BUFFER_SIZE];
    sprintf(tam_char, "%d", tamanho); // Converte tamanho para string

    // Obtém a data/hora atual no formato GMT para o cabeçalho HTTP
    time_t data;
    struct tm *info_tempo;
    time(&data);
    info_tempo = gmtime(&data);

    char data_formatada[128];
    strftime(data_formatada, sizeof(data_formatada), "Date: %a, %d %b %Y %H:%M:%S GMT", info_tempo);

    // Aloca memória para a resposta HTTP (cabeçalhos + conteúdo)
    char *resposta = (char *)malloc(tamanho + 512);

    // Monta o cabeçalho de acordo com o código de resposta
    if (codigo == 200)
    {
        strcpy(resposta, "HTTP/1.1 200 OK\r\n");
    }
    else if (codigo == 404)
    {
        strcpy(resposta, "HTTP/1.1 404 Not Found\r\n");
    }
    else
    {
        strcpy(resposta, "HTTP/1.1 500 Internal Server Error\r\n");
    }

    // Adiciona os demais cabeçalhos HTTP
    strcat(resposta, data_formatada);
    strcat(resposta, "\r\n");
    strcat(resposta, "Server: brenim\r\n");
    strcat(resposta, "Accept-Ranges: bytes\r\n");
    strcat(resposta, "Content-Length: ");
    strcat(resposta, tam_char);
    strcat(resposta, "\r\n");
    strcat(resposta, "Connection: close\r\n");
    strcat(resposta, "Content-Type: text/html\r\n\r\n");
    strcat(resposta, pagina); // Adiciona o conteúdo HTML
    strcat(resposta, "\r\n\r\n");

    free(pagina); // Libera memória do conteúdo HTML
    return resposta; // Retorna resposta completa
}

/*
    Função: processar_requisicao
    ----------------------------
    Processa a requisição HTTP recebida e envia a resposta ao cliente.
    - Se o método for GET, serve o arquivo solicitado.
    - Se o método não for GET, retorna erro 501 (Not Implemented).
*/
void processar_requisicao(int socket, requisicao *req)
{
    char *resposta;

    if (strcmp(req->metodo, "GET") == 0) // Se método for GET
    {
        if (strcmp(req->caminho, "/") == 0) // Se caminho for raiz
        {
            resposta = pagina_resposta("index.html"); // Retorna index.html
        }
        else
        {
            retirar_barra_caminho(req->caminho); // Remove barra inicial
            resposta = pagina_resposta(req->caminho); // Retorna arquivo solicitado
        }

        if (resposta != NULL)
        {
            send(socket, resposta, strlen(resposta), 0); // Envia resposta ao cliente
            free(resposta); // Libera memória
        }
    }
    else // Se método não for GET
    {
        printf("501 (Método não implementado)\n");

        // Corpo HTML para erro 501
        char *corpo_html_501 = "<html><body><h1>501 Not Implemented</h1><p>Método não suportado.</p></body></html>";
        long tamanho_corpo = strlen(corpo_html_501);

        // Monta resposta HTTP para erro 501
        char *resposta_501 = malloc(tamanho_corpo + 256);
        sprintf(resposta_501, 
            "HTTP/1.1 501 Not Implemented\r\n"
            "Server: brenim\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %ld\r\n" 
            "Connection: close\r\n"
            "\r\n" 
            "%s",  
            tamanho_corpo, 
            corpo_html_501
        );
        send(socket, resposta_501, strlen(resposta_501), 0); // Envia resposta 501
        free(resposta_501); // Libera memória
    }
}

/*
    Função: tratar_chamado
    ----------------------
    Extrai os três primeiros campos da mensagem HTTP recebida:
    - Método (ex: GET)
    - Caminho (ex: /index.html)
    - Protocolo (ex: HTTP/1.1)
    Utiliza sscanf para separar os campos da primeira linha da requisição.
*/
requisicao tratar_chamado(char mensagem[])
{
    requisicao req;
    req.metodo[0] = '\0';
    req.caminho[0] = '\0';
    req.protocolo[0] = '\0';
    // Lê os três primeiros campos da mensagem HTTP
    sscanf(mensagem, "%49s %49s %49s", req.metodo, req.caminho, req.protocolo);
    return req;
}

/*
    Função: tratar_conexao
    ----------------------
    Função executada por cada thread para tratar a conexão de um cliente.
    - Recebe a requisição HTTP do cliente.
    - Processa e responde de acordo com o método e caminho.
    - Fecha o socket ao final.
    - Decrementa o contador de conexões usando mutex.
*/
void *tratar_conexao(void *argg)
{
    int arg = *((int *)argg); // Pega posição do socket no array
    free(argg); // Libera memória do argumento

    int socket = new_socket[arg]; // Obtém socket do cliente
    char mensagem[BUFFER_SIZE];
    requisicao req;
    int len;

    char mensagem_completa[2048] = {0}; 
    // Recebe a mensagem HTTP do cliente
    len = recv(socket, mensagem_completa, sizeof(mensagem_completa) - 1, 0); //Retorna tamanho da mensagem em bytes

    if (len > 0)
    {
        mensagem_completa[len] = '\0'; // Finaliza string
        
        // Exibe a requisição HTTP recebida
        printf("\n--- INÍCIO DA REQUISIÇÃO HTTP ---\n%s--- FIM DA REQUISIÇÃO HTTP ---\n\n", mensagem_completa);

        req = tratar_chamado(mensagem_completa); // Extrai dados da requisição
        
        printf("  >> Requisição resumida: %s %s\n", req.metodo, req.caminho);

        processar_requisicao(socket, &req); // Processa e responde
    }

    close(socket); // Fecha conexão com cliente

    // Protege acesso ao contador de conexões usando mutex
    pthread_mutex_lock(&mutex);
    qtde_sockets--;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

/*
    Função: main
    ------------
    Função principal do servidor.
    - Cria o socket do servidor.
    - Associa o socket à porta definida.
    - Coloca o socket em modo de escuta.
    - Aceita conexões de clientes em loop infinito.
    - Para cada conexão, cria uma thread para tratar a requisição.
    - Utiliza mutex para proteger o contador de conexões simultâneas.
*/
int main(int argc, char *argv[])
{
    int s, c, pos;
    struct sockaddr_in servidor, cliente;

    printf("oi\n\n"); // Mensagem inicial

    // Cria o socket TCP (AF_INET = IPv4, SOCK_STREAM = TCP)
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro");
        exit(EXIT_FAILURE);
    }

    // Configura a estrutura do servidor
    servidor.sin_family = AF_INET; // Família de endereços IPv4
    servidor.sin_addr.s_addr = INADDR_ANY; // Aceita conexões de qualquer IP
    servidor.sin_port = htons(PORTA); // Define porta do servidor

    // Associa o socket à porta e endereço
    if (bind(s, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
    {
        perror("Erro");
        close(s);
        exit(EXIT_FAILURE);
    }

    // Coloca o socket em modo de escuta (aguardando conexões)
    listen(s, 3); // três conexões na fila de espera

    printf("Servidor escutando na porta %d. Servindo arquivos de %s\n", PORTA, LOCATION);
    c = sizeof(struct sockaddr_in);

    // Loop principal do servidor: aceita conexões e cria threads para cada cliente
    while (1)
    {
        int cliente_socket = accept(s, (struct sockaddr *)&cliente, (socklen_t *)&c); // Aceita conexão de cliente
        if (cliente_socket < 0)
        {
            printf("Erro ao aceitar conexao\n");
        }
        else
        {
            puts("Conexao aceita\n");
            printf("Cliente conectado: %s:%d\n", inet_ntoa(cliente.sin_addr), ntohs(cliente.sin_port));
            pthread_mutex_lock(&mutex); // Protege acesso ao contador
            if (qtde_sockets < QTE_SOCKETS) // Verifica limite de conexões
            {
                pos = qtde_sockets;
                new_socket[pos] = cliente_socket; // Armazena socket do cliente
                qtde_sockets++;
                pthread_mutex_unlock(&mutex);

                pthread_t sniffet_thread;
                int *arg = malloc(sizeof(int)); // Aloca memória para argumento da thread
                *arg = pos;
                // Cria uma thread para tratar a conexão do cliente
                if (pthread_create(&sniffet_thread, NULL, tratar_conexao, arg) != 0)
                {
                    perror("Erro ao criar thread");
                    close(cliente_socket);
                }
            }
        }
    }

    close(s); // Fecha socket do servidor
    pthread_mutex_destroy(&mutex); // Destroi mutex
    return 0;
}