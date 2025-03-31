from flask import Flask, render_template, Response, jsonify
import socket
import threading
import time
import json

# Configurações do servidor TCP
TCP_HOST = '0.0.0.0'  # Escuta em todas as interfaces
TCP_PORT = 5000       # Porta do servidor TCP

# Configurações do servidor Flask
WEB_HOST = '0.0.0.0'  # Escuta em todas as interfaces
WEB_PORT = 8080       # Porta do servidor Flask

# Lista para armazenar os dados recebidos
received_data = []

# Configuração do Flask
app = Flask(__name__)

# Rota para a interface web
@app.route('/')
def index():
    return render_template('index.html')

# Rota para obter dados em formato JSON
@app.route('/data')
def get_data():
    return jsonify(received_data)

# Rota para o stream de dados (SSE)
@app.route('/stream')
def stream():
    def event_stream():
        while True:
            if received_data:  # Se houver novos dados
                data = received_data[-1]  # Pega o último dado recebido
                yield f"data: {json.dumps(data)}\n\n"  # Envia o dado para o cliente
            time.sleep(0.1)  # Aguarda 0.1 segundo antes de verificar novamente
    return Response(event_stream(), content_type='text/event-stream')

# Função para o servidor TCP
def tcp_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((TCP_HOST, TCP_PORT))
        s.listen()
        print(f"Servidor TCP escutando em {TCP_HOST}:{TCP_PORT}")
        while True:
            conn, addr = s.accept()
            with conn:
                print(f"Conectado por {addr}")
                while True:
                    data = conn.recv(1024)
                    if not data:
                        break
                    try:
                        # Parseia os dados JSON recebidos
                        decoded_data = json.loads(data.decode())
                        print(f"Dados recebidos: {decoded_data}")
                        
                        # Verifica se a nova nota é diferente das duas últimas
                        if len(received_data) < 2 or (decoded_data != received_data[-1] and decoded_data != received_data[-2]):
                            received_data.append(decoded_data)
                        else:
                            print("Nota repetida, não exibindo.")
                            
                    except json.JSONDecodeError:
                        print("Erro ao decodificar JSON")
                print(f"Conexão com {addr} fechada")

# Inicia o servidor TCP em uma thread separada
tcp_thread = threading.Thread(target=tcp_server)
tcp_thread.daemon = True  # Permite que a thread seja encerrada quando o programa principal terminar
tcp_thread.start()

# Inicia o servidor web Flask
if __name__ == '__main__':
    app.run(host=WEB_HOST, port=WEB_PORT)
