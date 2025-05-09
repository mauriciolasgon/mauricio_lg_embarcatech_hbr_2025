
# Projetos de Sistemas Embarcados - EmbarcaTech 2025

Autor: **Mauricio Lasca Gonçales**

Curso: Residência Tecnológica em Sistemas Embarcados

Instituição: EmbarcaTech - HBr

Campinas, Maio de 2025

---

# 🎲 **Simulação de Galton Board com Display OLED SSD1306**

## 📚 **Descrição do Projeto**

Este projeto implementa a simulação de uma **Galton Board** utilizando um display OLED SSD1306 e o microcontrolador RP2040 (placa BitDogLab). A simulação ilustra o comportamento estatístico de uma distribuição normal, demonstrando que a maior concentração de bolinhas ocorre nas canaletas centrais, conforme a Lei dos Grandes Números.

---

## 🖥️ **Funcionamento da Simulação**

- Ao iniciar o programa, um número pré-definido de bolinhas começa a cair, colidindo com os pinos e preenchendo as canaletas.
- O número de bolinhas é inicialmente configurado como **1000** para facilitar a visualização da distribuição estatística no display.
- Na tela inicial, as canaletas possuem uma limitação de exibição e não conseguem representar um grande número de bolinhas. Por isso, ao final da queda, é possível pressionar o **botão A** para alternar para uma nova tela que exibe a **quantidade real de bolinhas em cada canaleta**.
- **Limitação de Visualização em Tempo Real:**
  - Com até **20 bolinhas**, é possível acompanhar o movimento individual de cada uma no display.
  - Acima de **20 bolinhas**, a exibição em tempo real é desabilitada, sendo mostrado apenas quando as bolinhas atingem as canaletas.
- Após todas as bolinhas caírem, pressionando o **botão A da placa BitDogLab**, uma nova tela exibe a **quantidade de bolinhas por canaleta** e a **distribuição final**.
- Após visualizar a distribuição final, pressionando novamente o **botão A**, o programa reinicia a simulação.

---

## ⚙️ **Configurações**

- O número total de bolinhas é definido na função `main`, através da variável `num_bolas`.

> **Exemplo:**  
> Altere o valor de `num_bolas` diretamente na `main` para visualizar o impacto de diferentes quantidades de bolinhas na distribuição.

---

## 🗃️ **Estruturas Utilizadas**

- **Struct `bola`**: Gerencia o estado individual de cada bolinha (posição, velocidade, etc.).
- **Struct `canaleta`**: Gerencia a quantidade de bolinhas em cada canaleta, como também qual deve ser o bit acesso e em qual coluna naquela canaleta.

---

## 📏 **Especificações Técnicas**

- **Display:** OLED SSD1306  
- **Resolução:** 128x64 pixels  
- **Formato do Tabuleiro:** 128x64 (largura x altura)  
- **Gerador de Aleatoriedade:**  
  Utiliza o **ADC do sensor de temperatura** do RP2040, capturando o ruído no bit menos significativo (LSB) para gerar escolhas binárias.

---

## 🔢 **Exibição de Números**

- Implementado um **bitmap customizado para os números de 0 a 9**, com escala reduzida em relação às fontes padrão da biblioteca `ssd1306`.
- Essa técnica permite exibir de forma compacta a quantidade de bolinhas em cada canaleta na tela de distribuição final.

---

## 📄 **Organização do Código**

1. **Declaração de Structs e Variáveis Globais**  
2. **Definição das Funções Utilizadas**  
3. **Função `main` com Fluxo de Execução**  
   - A cada iteração do loop principal, uma nova bolinha é movimentada.
   - Caso haja um gargalo na posição inicial, a bolinha mais avançada inicia o movimento, liberando espaço para as próximas.
4. **Implementação das Funções Declaradas**  

---

---

## 📜 Licença
MIT License - MIT GPL-3.0.

