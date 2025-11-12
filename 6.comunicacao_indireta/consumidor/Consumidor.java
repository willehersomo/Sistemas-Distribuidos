import org.apache.kafka.clients.consumer.ConsumerConfig;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.apache.kafka.common.serialization.StringDeserializer;

import java.time.Duration;
import java.util.Collections;
import java.util.Properties;

public class Consumidor{
    public static void main(String[] args) {
        Properties propriedades = new Properties();
        
        propriedades.put(ConsumerConfig.BOOTSTRAP_SERVERS_CONFIG, "kafka:9092");         
        propriedades.put(ConsumerConfig.GROUP_ID_CONFIG, "grupo_consumidor");
        propriedades.put(ConsumerConfig.KEY_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());
        propriedades.put(ConsumerConfig.VALUE_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());
        propriedades.put(ConsumerConfig.AUTO_OFFSET_RESET_CONFIG, "earliest");
        propriedades.put(ConsumerConfig.CLIENT_ID_CONFIG, "consumidor-java-1");

        KafkaConsumer<String, String> consumidor = new KafkaConsumer<>(propriedades);
        consumidor.subscribe(Collections.singletonList("exemplo"));

        System.out.println("Consumidor Ativo e conectado a kafka:9092");
        while (true) {
            ConsumerRecords<String, String> registros = consumidor.poll(Duration.ofMillis(100));
            for (ConsumerRecord<String, String> registro : registros) {
                System.out.printf("Recebido: chave=%s, valor=%s, partição=%d, offset=%d\n",
                    registro.key(), registro.value(), registro.partition(), registro.offset());
            }
        }
    }
}