use log::info;
use std::{
    sync::{mpsc, Arc, Mutex},
    thread,
};

type Tarefa = Box<dyn FnOnce() + Send + 'static>; 
//tarefa só pode acontecer uma vez (fnonce), pode ser enviada entre threads (send), roda no tempo da apliacação


pub struct PoolThreads {
    trabalhadores: Vec<Trabalhador>,
    remetente: Option<mpsc::Sender<Tarefa>>, //multi producer, single consumer
}

impl PoolThreads {
    pub fn nova(tamanho: usize) -> PoolThreads {
        assert!(tamanho > 0, "O tamanho do pool de threads deve ser maior que zero.");

        let (remetente, receptor) = mpsc::channel();
        let receptor = Arc::new(Mutex::new(receptor));

        let mut trabalhadores = Vec::with_capacity(tamanho);

        for id in 0..tamanho {
            trabalhadores.push(Trabalhador::novo(id, Arc::clone(&receptor)));
        }

        PoolThreads {
            trabalhadores,
            remetente: Some(remetente),
        }
    }

    pub fn executar<F>(&self, f: F)
    where
        F: FnOnce() + Send + 'static,
    {
        let tarefa = Box::new(f);
        self.remetente.as_ref().unwrap().send(tarefa).unwrap();
    }
}



impl Drop for PoolThreads {
    fn drop(&mut self) {
        drop(self.remetente.take());

        for trabalhador in &mut self.trabalhadores {
            info!("Desligando trabalhador {}", trabalhador.id);

            if let Some(thread) = trabalhador.thread.take() {
                thread.join().unwrap();
            }
        }
    }
}


struct Trabalhador {
    id: usize,
    thread: Option<thread::JoinHandle<()>>,
}

impl Trabalhador {
    fn novo(id: usize, receptor: Arc<Mutex<mpsc::Receiver<Tarefa>>>) -> Trabalhador {
        let thread = thread::spawn(move || loop {
            let mensagem = receptor.lock().unwrap().recv();

            match mensagem {
                Ok(tarefa) => {
                    info!("Trabalhador {} recebeu uma tarefa; executando.", id);
                    tarefa();
                }
                Err(_) => {
                    info!("Trabalhador {} desconectado; encerrando.", id);
                    break;
                }
            }
        });

        Trabalhador {
            id,
            thread: Some(thread),
        }
    }
}

