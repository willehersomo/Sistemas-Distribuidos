from kafka import KafkaProducer
import json
import time

produtor = KafkaProducer(
    bootstrap_servers='kafka:9092',
    value_serializer=lambda valor: json.dumps(valor).encode('utf-8'),
    key_serializer=lambda chave: str(chave).encode('utf-8')
)

contador = 0
while True:
    chave_particao = f"chave_{contador % 3}" 
    mensagem = {'numero': contador, 'texto': f"Mas veja {contador}"}
    
    produtor.send('exemplo', key=chave_particao, value=mensagem)
    
    print(f'Enviada: {mensagem} | chave: {chave_particao}')
    contador += 1
    time.sleep(5)

produtor.flush()