import requests
import xml.etree.ElementTree as ET
import sys

URL_SERVICO = "http://127.0.0.1:8080/soap"
NAMESPACE_SERVICO = "http://bestiario.rpg.com/"

MODELO_XML_PESQUISA = f"""
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns="{NAMESPACE_SERVICO}">
  <soap:Body>
    <ns:PesquisarClasses>
       <Nome>{{nome}}</Nome>
       <AtributoPrimario>{{atributoPrimario}}</AtributoPrimario>
    </ns:PesquisarClasses>
  </soap:Body>
</soap:Envelope>
"""

MODELO_XML_TODAS = f"""
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns="{NAMESPACE_SERVICO}">
  <soap:Body>
    <ns:ListarTodasAsClasses/>
  </soap:Body>
</soap:Envelope>
"""

def chamar_servico_soap(carga_xml, acao_soap):
    print("\nEnviando requisição para:", URL_SERVICO)
    
    print("\n--- INÍCIO XML DE REQUISIÇÃO ---")
    print(carga_xml)
    print("--- FIM XML DE REQUISIÇÃO ---\n")
    

    headers = {
        'Content-Type': 'text/xml; charset=utf-8',
        'SOAPAction': f'"{NAMESPACE_SERVICO}{acao_soap}"'
    }

    try:
        response = requests.post(URL_SERVICO, data=carga_xml.encode('utf-8'), headers=headers, timeout=10)
        
        print(f"--- Código de Status HTTP: {response.status_code} ---")
        
        xml_resposta = response.text 
        
        print(xml_resposta +"\n\n")
        
        if response.status_code == 200:
            print("Resposta de Sucesso (Processada):") 
            processar_resposta_sucesso(xml_resposta)
        else:
            print("Resposta de Falha (Processada):")
            processar_resposta_falha(xml_resposta)

    except requests.exceptions.RequestException as e:
        print(f"\n[ERRO DE REDE]: Falha ao conectar ao servidor.")
        print(f"Detalhe: {e}")
        print("Verifique se o servidor Java está rodando.")

def processar_resposta_sucesso(xml_resposta):
    try:
        namespaces = {
            'soap': 'http://schemas.xmlsoap.org/soap/envelope/',
            'ns': NAMESPACE_SERVICO 
        }
        
        root = ET.fromstring(xml_resposta)
        
        tag_resposta = ".//ns:PesquisarClassesResponse"
        if root.find(tag_resposta, namespaces) is None:
            tag_resposta = ".//ns:ListarTodasAsClassesResponse"

        classes_encontradas = root.findall(tag_resposta + "/return", namespaces)
        
        if not classes_encontradas:
            print("\nNenhuma classe encontrada na resposta.")
            return

        print("\nClasses Encontradas:")
        for i, classe_node in enumerate(classes_encontradas):
            nome_node = classe_node.find('ns:Nome', namespaces)
            dado_vida_node = classe_node.find('ns:DadoVida', namespaces)
            atributo_node = classe_node.find('ns:AtributoPrimario', namespaces)
            foco_node = classe_node.find('ns:Foco', namespaces)

            nome = nome_node.text if nome_node is not None else "N/A"
            dado_vida = dado_vida_node.text if dado_vida_node is not None else "N/A"
            atributo = atributo_node.text if atributo_node is not None else "N/A"
            foco = foco_node.text if foco_node is not None else "N/A"
            
            print(f"--- Classe {i+1} ---")
            print(f"  Nome: {nome}")
            print(f"  Dado de Vida: {dado_vida}")
            print(f"  Atributo: {atributo}")
            print(f"  Foco: {foco}")

    except ET.ParseError as e:
        print(f"Erro ao analisar o XML de resposta: {e}")
        print("XML Bruto:", xml_resposta)

def processar_resposta_falha(xml_resposta):
    try:
        namespaces = {'soap': 'http://schemas.xmlsoap.org/soap/envelope/'}
        root = ET.fromstring(xml_resposta)
        
        faultstring = root.find('.//faultstring', namespaces)
        
        if faultstring is not None:
            print(f"\nErro do Servidor: {faultstring.text}")
        else:
            print("\nO servidor retornou um erro, mas não foi possível encontrar <faultstring>.")
            print("XML Bruto:", xml_resposta)
            
    except ET.ParseError as e:
        print(f"Erro ao analisar o XML de falha: {e}")
        print("XML Bruto:", xml_resposta)

def executar_pesquisa():
    nome = input("Digite o Nome (ou deixe em branco): ").strip()
    atributo = input("Digite o Atributo Primário (ou deixe em branco): ").strip()
    
    carga_xml = MODELO_XML_PESQUISA.format(nome=nome, atributoPrimario=atributo)
    
    chamar_servico_soap(carga_xml, "PesquisarClasses") 

def executar_listar_todas():
    chamar_servico_soap(MODELO_XML_TODAS, "ListarTodasAsClasses")

def main():
    while True:
        print("\n--- Cliente SOAP em Python ---")
        print("1. Pesquisar Classes")
        print("2. Listar Todas as Classes")
        print("3. Sair")
        escolha = input("Escolha uma opção: ")

        if escolha == '1':
            executar_pesquisa()
        elif escolha == '2':
            executar_listar_todas()
        elif escolha == '3':
            print("Saindo.")
            break
        else:
            print("Opção inválida. Tente novamente.")

if __name__ == "__main__":
    main()