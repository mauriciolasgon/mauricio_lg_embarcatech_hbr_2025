## Rastreador de Notas Musicais

## Objetivo

O Rastreador de Notas Musicais é uma ferramenta projetada para detectar e identificar notas musicais em sons ambientes por meio da análise de frequência utilizando a Transformada Rápida de Fourier (FFT). O projeto incorpora a implementação do algoritmo FFT desenvolvida por Van Hunter Adams, permitindo a conversão eficiente de sinais de áudio em frequências e a identificação das notas musicais em tempo real.

Essa funcionalidade torna a ferramenta especialmente útil para músicos, pesquisadores e entusiastas de áudio digital que desejam analisar ou acompanhar sons de forma precisa.

Para fins de teste, foi desenvolvido um servidor Flask que recebe os dados do dispositivo e os exibe na tela, facilitando a visualização e interpretação das informações processadas.

## Componentes Utilizados

Este projeto foi desenvolvido utilizando o hardware da BitDogLab, explorando os recursos do Raspberry Pi Pico W e seus periféricos para captura, processamento e exibição das informações. Os principais componentes são:

Botão: Utilizado para gerenciar a conexão WIFI.

Microfone: Captura os sons do ambiente para análise.

Conversor Analógico-Digital (ADC): Converte os sinais analógicos do microfone em dados digitais.

Direct Memory Access (DMA): Gerencia a transferência de dados de forma eficiente, otimizando o desempenho.

Dual-core do Raspberry Pi Pico W: Distribui a carga de trabalho entre os dois núcleos para processamento paralelo.

Display OLED: Exibe o status da conexão WIFI.

Módulo Wi-Fi: Permite a comunicação com um servidor para armazenar e processar os dados remotamente.

## Pinagem

Os seguintes pinos do Raspberry Pi Pico W foram utilizados:

GPIO 6 → Botão.

GPIO 26 (ADC Canal 2) → Entrada do microfone.

GPIOs 14 e 15 (I2C) → Comunicação com o display OLED.

## Resultados e Conclusões


O Rastreador de Notas Musicais consegue identificar notas musicais com base em sua frequência. No entanto, algumas limitações foram observadas:

Para frequências mais baixas, os harmônicos podem ser identificados erroneamente em detrimento da frequência fundamental.

A conexão com o servidor ainda apresenta instabilidades, necessitando maior robustez na implementação da comunicação Wi-Fi e conexão com o servidor.


