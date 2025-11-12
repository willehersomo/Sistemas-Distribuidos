mod validador;

use chrono::Utc;
use log::{info, warn, error};
use std::io::{Read, Write, Error as IoError};
use std::net::{TcpListener, TcpStream, SocketAddr};
use std::sync::{Arc, Mutex};
use threadpool::ThreadPool;

const HOST: &str = "127.0.0.1";
const PORTA: u16 = 8080;
const MAX_THREADS_TRABALHADORAS: usize = 4;
const MAX_CONEXOES_ATIVAS: usize = 10;

const TIPO_DOC_CPF: u8 = 0x01;
const TIPO_DOC_CNPJ: u8 = 0x02;

const STATUS_VALIDO: u8 = 0x01;
const STATUS_INVALIDO: u8 = 0x00;
const STATUS_ERRO: u8 = 0xFF;

struct Requisicao {
    tipo_documento: u8,
    documento_str: String,
}


fn logar_resposta_http(end_remoto: SocketAddr, status_resposta: u8) {
    let (linha_status_http, corpo_resposta) = match status_resposta {
        STATUS_VALIDO => ("200 OK", "{\"status\":\"valido\"}"),
        STATUS_INVALIDO => ("400 Bad Request", "{\"status\":\"invalido\"}"),
        _ => ("500 Internal Server Error", "{\"status\":\"erro\"}"),
    };

    let data = Utc::now().to_rfc2822();

    info!("[{}] >> HTTP/1.1 {}", end_remoto, linha_status_http);
    info!("[{}] >> Date: {}", end_remoto, data);
    info!("[{}] >> Server: Rust-Validador/0.3.0", end_remoto);
    info!("[{}] >> Content-Type: application/json", end_remoto);
    info!("[{}] >> Content-Length: {}", end_remoto, corpo_resposta.len());
    info!("[{}] >> ", end_remoto);
    info!("[{}] >> {}", end_remoto, corpo_resposta);
}

fn processar_validacao(requisicao: &Requisicao) -> u8 {
    match requisicao.tipo_documento {
        TIPO_DOC_CPF => {
            if validador::e_cpf_valido(&requisicao.documento_str) { STATUS_VALIDO } else { STATUS_INVALIDO }
        }
        TIPO_DOC_CNPJ => {
            if validador::e_cnpj_valido(&requisicao.documento_str) { STATUS_VALIDO } else { STATUS_INVALIDO }
        }
        _ => {
            warn!("Tipo de documento desconhecido: {}", requisicao.tipo_documento);
            STATUS_ERRO
        }
    }
}


fn logar_requisicao_http(end_remoto: SocketAddr, requisicao: &Requisicao) {
    let tipo_doc_str = match requisicao.tipo_documento {
        TIPO_DOC_CPF => "cpf",
        TIPO_DOC_CNPJ => "cnpj",
        _ => "desconhecido",
    };
    info!("[{}] \"POST /validar/{} HTTP/1.1\" Body: \"{}\"", end_remoto, tipo_doc_str, requisicao.documento_str);
}

fn ler_requisicao(fluxo: &mut TcpStream) -> Result<Requisicao, IoError> {
    let mut cabecalho = [0u8; 3];
    fluxo.read_exact(&mut cabecalho)?;
    
    let tipo_documento = cabecalho[0];
    let tamanho_payload = u16::from_be_bytes([cabecalho[1], cabecalho[2]]) as usize; 
    //o artigo que eu li falou que o ideal é usar o padrão network byte order (BigEndian)
    
    if tamanho_payload == 0 || tamanho_payload > 50 { //pro caba não estourar meu buffer
        return Err(IoError::new(std::io::ErrorKind::InvalidData, "Tamanho de payload inválido"));
    }

    let mut buffer_payload = vec![0u8; tamanho_payload];
    fluxo.read_exact(&mut buffer_payload)?;

    let documento_str = String::from_utf8(buffer_payload)
        .map_err(|_| IoError::new(std::io::ErrorKind::InvalidData, "Payload não é UTF-8 válido"))?;
    
    Ok(Requisicao { tipo_documento, documento_str })
}


fn tratar_conexao(mut fluxo: TcpStream, end_remoto: SocketAddr) -> Result<(), IoError> {
    let requisicao = match ler_requisicao(&mut fluxo) {
        Ok(req) => req,
        Err(e) => {
            warn!("[{}] Erro ao ler requisição: {}", end_remoto, e);
            fluxo.write_all(&[STATUS_ERRO])?;
            return Ok(());
        }
    };
    
    logar_requisicao_http(end_remoto, &requisicao);

    let status_resposta = processar_validacao(&requisicao);
    
    logar_resposta_http(end_remoto, status_resposta);

    fluxo.write_all(&[status_resposta])?;
    Ok(())
}

fn executar_loop_de_escuta(escutador: TcpListener, pool_threads: ThreadPool, conexoes_ativas: Arc<Mutex<usize>>) {
    
    for fluxo in escutador.incoming() { //roda até conectar 
        let fluxo = match fluxo {
            Ok(f) => f,
            Err(e) => {
                warn!("Erro ao aceitar conexão de entrada: {}", e);
                continue; 
            }
        };

        let end_remoto = fluxo.peer_addr().unwrap_or_else(|_| "0.0.0.0:0".parse().unwrap()); //Cliente, caso não ip ficticio

        let mut num_conexoes = conexoes_ativas.lock().unwrap();
        if *num_conexoes >= MAX_CONEXOES_ATIVAS {
            warn!(
                "Limite de conexões atingido ({}). Rejeitando nova conexão de {}.",
                MAX_CONEXOES_ATIVAS, end_remoto
            );
            continue;
        }

        *num_conexoes += 1;
        info!(
            "Conexão aceita de: {}. Conexões ativas: {}",
            end_remoto, *num_conexoes
        );
        
        let clone_conexoes = Arc::clone(&conexoes_ativas);
        
        pool_threads.execute(move || {
            if let Err(e) = tratar_conexao(fluxo, end_remoto) {
                error!("Erro ao manipular conexão de {}: {}", end_remoto, e);
            }
            let mut num = clone_conexoes.lock().unwrap();
            *num -= 1;
            info!("Conexão com {} encerrada. Conexões ativas: {}", end_remoto, *num);
            info!("----------------------------------------------");
        });
    }
} 


fn iniciar_servidor() -> Result<(), IoError> {
    env_logger::init();

    let escutador = TcpListener::bind(format!("{}:{}", HOST, PORTA))?;
    let pool_threads = ThreadPool::new(MAX_THREADS_TRABALHADORAS);
    let conexoes_ativas = Arc::new(Mutex::new(0));

    info!("Servidor escutando em {}:{}", HOST, PORTA);
    info!("----------------------------------------------");

    executar_loop_de_escuta(escutador, pool_threads, conexoes_ativas);

    Ok(())
}

fn main() {
    if let Err(e) = iniciar_servidor() {
        error!("Erro fatal que impediu a inicialização do servidor: {}", e);
    }
}


