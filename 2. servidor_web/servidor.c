#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

//http://10.117.67.200:8080/

#define PORTA 8080
#define BUFFER_SIZE 2048
#define QTE_SOCKETS 5

#define LOCATION "/tmp/www/"


int new_socket[QTE_SOCKETS];
int qtde_sockets = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    char metodo[50];    // Get
    char caminho[50];   // Index.html
    char protocolo[50]; // HTTP/1.1
} requisicao;

void retirar_barra_caminho(char caminho[])
{
    int i;
    int tamanho = strlen(caminho);

    if (tamanho > 0 && caminho[0] == '/')
    {
        for (i = 0; i < tamanho; i++)
        {
            caminho[i] = caminho[i + 1];
        }
    }
}

char *ler_arquivo(char nome_arquivo[])
{
    char caminho_completo[512];
    snprintf(caminho_completo, sizeof(caminho_completo), "%s%s", LOCATION, nome_arquivo);

    FILE *arquivo = fopen(caminho_completo, "r");
    if (arquivo == NULL)
    {
        perror("Erro ao abrir arquivo");
        printf("Caminho tentado: %s\n", caminho_completo);
        return NULL;
    }
    else
    {
        fseek(arquivo, 0, SEEK_END);
        long tamanho = ftell(arquivo);
        fseek(arquivo, 0, SEEK_SET);

        char *conteudo = (char *)malloc(tamanho + 1);
        if (conteudo == NULL)
        {
            fclose(arquivo);
            return NULL;
        }

        fread(conteudo, 1, tamanho, arquivo);
        conteudo[tamanho] = '\0';
        fclose(arquivo);
        return conteudo;
    }
}

char *pagina_resposta(char nome_arquivo[])
{
    int codigo = 200; // sucesso
    char *pagina = ler_arquivo(nome_arquivo);
    if (pagina == NULL)
    {
        pagina = ler_arquivo("404.html");
        codigo = 404;
    }
    

    int tamanho = strlen(pagina);
    char tam_char[BUFFER_SIZE];
    sprintf(tam_char, "%d", tamanho); // convertido em texto para usar no cabeçalho

    time_t data;
    struct tm *info_tempo;
    time(&data);
    info_tempo = gmtime(&data);

    char data_formatada[128];
    strftime(data_formatada, sizeof(data_formatada), "Date: %a, %d %b %Y %H:%M:%S GMT", info_tempo);

    char *resposta = (char *)malloc(tamanho + 512);

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

    strcat(resposta, data_formatada);
    strcat(resposta, "\r\n");
    strcat(resposta, "Server: brenim\r\n");
    strcat(resposta, "Accept-Ranges: bytes\r\n");
    strcat(resposta, "Content-Length: ");
    strcat(resposta, tam_char);
    strcat(resposta, "\r\n");
    strcat(resposta, "Connection: close\r\n");
    strcat(resposta, "Content-Type: text/html\r\n\r\n");
    strcat(resposta, pagina);
    strcat(resposta, "\r\n\r\n");

    free(pagina);
    return resposta;
}


void processar_requisicao(int socket, requisicao *req)
{
    char *resposta;

    if (strcmp(req->metodo, "GET") == 0)
    {
        if (strcmp(req->caminho, "/") == 0)
        {
            resposta = pagina_resposta("index.html");
        }
        else
        {
            retirar_barra_caminho(req->caminho);
            resposta = pagina_resposta(req->caminho);
        }

        if (resposta != NULL)
        {
            send(socket, resposta, strlen(resposta), 0);
            free(resposta);
        }
    }
    else
    {
        printf("501 (Método não implementado)\n");

        char *corpo_html_501 = "<html><body><h1>501 Not Implemented</h1><p>Método não suportado.</p></body></html>";
        
        long tamanho_corpo = strlen(corpo_html_501);
    
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
        send(socket, resposta_501, strlen(resposta_501), 0);
        free(resposta_501);
    }
}


requisicao tratar_chamado(char mensagem[])
{
    requisicao req;
    req.metodo[0] = '\0';
    req.caminho[0] = '\0';
    req.protocolo[0] = '\0';
    sscanf(mensagem, "%49s %49s %49s", req.metodo, req.caminho, req.protocolo);
    return req;
}

void *tratar_conexao(void *argg)
{
    int arg = *((int *)argg); //só pra da free no argg depois
    free(argg);

    int socket = new_socket[arg];
    char mensagem[BUFFER_SIZE];
    requisicao req;
    int len;

    char mensagem_completa[2048] = {0}; 
    len = recv(socket, mensagem_completa, sizeof(mensagem_completa) - 1, 0); //recebe dados socket (bytes)

    if (len > 0)
    {
        mensagem_completa[len] = '\0'; 
        
        printf("\n--- INÍCIO DA REQUISIÇÃO HTTP ---\n%s--- FIM DA REQUISIÇÃO HTTP ---\n\n", mensagem_completa);

        req = tratar_chamado(mensagem_completa);
        
        printf("  >> Requisição resumida: %s %s\n", req.metodo, req.caminho);

        processar_requisicao(socket, &req);
    }

    close(socket);
    pthread_mutex_lock(&mutex);
    qtde_sockets--;
    pthread_mutex_unlock(&mutex);
    return NULL;
}


int main(int argc, char *argv[])
{
    int s, c, pos;
    struct sockaddr_in servidor, cliente;

    printf("oi\n\n");

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro");
        exit(EXIT_FAILURE);
    }

    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons(PORTA);

    if (bind(s, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
    {
        perror("Erro");
        close(s);
        exit(EXIT_FAILURE);
    }

    listen(s, 3);

    printf("Servidor escutando na porta %d. Servindo arquivos de %s\n", PORTA, LOCATION);
    c = sizeof(struct sockaddr_in);

    while (1)
    {
        int cliente_socket = accept(s, (struct sockaddr *)&cliente, (socklen_t *)&c);
        if (cliente_socket < 0)
        {
            printf("Erro ao aceitar conexao\n");
        }
        else
        {
            puts("Conexao aceita\n");
            printf("Cliente conectado: %s:%d\n", inet_ntoa(cliente.sin_addr), ntohs(cliente.sin_port));
            pthread_mutex_lock(&mutex);
            if (qtde_sockets < QTE_SOCKETS)
            {
                pos = qtde_sockets;
                new_socket[pos] = cliente_socket;
                qtde_sockets++;
                pthread_mutex_unlock(&mutex);

                pthread_t thread;
                int *arg = malloc(sizeof(int));
                *arg = pos;
                if (pthread_create(&thread, NULL, tratar_conexao, arg) != 0)
                {
                    perror("Erro ao criar thread");
                    close(cliente_socket);
                }
            }
        }
    }
    close(s);
    pthread_mutex_destroy(&mutex);
    return 0;
}