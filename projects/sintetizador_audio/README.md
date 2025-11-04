
# Projetos de Sistemas Embarcados - EmbarcaTech 2025

Autor: **Mauricio Lasca Gonçales**

Curso: Residência Tecnológica em Sistemas Embarcados

Instituição: EmbarcaTech - HBr

Campinas, Junho de 2025

---

## Descrição do Projeto  
Este projeto implementa um sintetizador de áudio usando a placa BitDogLab. Através de um microfone conectado ao canal ADC 2, botões, LEDs RGB, DMA, timers e PWM, o sistema grava até 3 segundos de áudio ambiente e reproduz o som capturado em tempo real por meio de buzzers. Quando o usuário pressiona o botão A, um LED azul acende indicando o início da gravação; o áudio é transferido do ADC para um buffer na memória via DMA, gerando interrupções periódicas a cada bloco de amostras para processamento. Após o término da gravação, uma flag dispara a reprodução: o programa agenda um timer para acionar o buzzer com PWM em frequência ótima, criando um efeito de sintetizador a partir dos dados do buffer.

## Funcionalidades Principais  
O firmware grava 0,25 s de áudio por bloco, reaglutinando blocos até totalizar 3 s de amostragem a 24 kHz, aplica um filtro IIR simples para suavização do sinal e escala a amplitude antes de gerar o PWM. Durante a gravação, o LED vermelho indica atividade, e ao final o LED azul pisca em sincronia com a reprodução. A divisão de clock do ADC e do PWM é configurada dinamicamente com base na frequência de amostragem, garantindo sincronização precisa entre captura e reprodução.

## Requisitos de Hardware  
É necessária uma placa BitDogLab compatível com o SDK do Raspberry Pi Pico, contendo:  
– Microfone analógico no pino ADC 26 (canal 2)  
– Buzzer no pino GPIO 21  
– LEDs RGB nos pinos GPIO 11, 12 e 13  
– Botão de gravação conectado ao GPIO 5 (botão A)  

## Configuração do Ambiente de Desenvolvimento  
Para compilar o projeto, instale o Raspberry Pi Pico SDK e chame `cmake` e `make` no diretório do repositório. As funções de inicialização configuram ADC, DMA, PWM e interrupções GPIO, garantindo que ao pressionar o botão a lógica de gravação e reprodução seja acionada corretamente. Não é necessário nenhum driver adicional além das bibliotecas padrão do Pico SDK.

## Estrutura do Código  
O arquivo principal contém:  
- `init_adc()`, que configura o ADC com FIFO e divisor de clock.  
- `init_dma()`, que aloca um canal DMA e prepara o controlador para capturar blocos de amostras.  
- `init_pwm_buzzers()`, que define o wrap do PWM e calcula o divisor de clock para manter a frequência de reprodução.  
- `gpio_callback()`, disparada na borda de subida do botão A, inicia o temporizador de gravação de 3 s.  
- `dma_handler()`, que remaneja endereços e contagem do DMA a cada bloco, atualizando o buffer e sinalizando a camada de aplicação.  
- `play_callback()`, chamada repetidamente pelo timer para ler o buffer, aplicar filtro IIR, ajustar a amplitude e gerar o valor PWM para o buzzer.

## Uso  
1. Conecte a placa BitDogLab ao computador via USB e selecione o build gerado.  
2. Ao pressionar o botão A, o LED azul acende e a gravação de 3 segundos começa.  
3. Após o término, a reprodução do áudio gravado inicia automaticamente, com o LED azul piscando em sincronia.  
4. Para repetir o ciclo, simplesmente pressione o botão A novamente.

---

## 📜 Licença
GNU GPL-3.0.

