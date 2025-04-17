# Leitura do Sensor de Temperatura Interno do RP2040

## Descrição da Atividade

Nesta atividade, foi desenvolvido um programa para **monitorar a temperatura interna** do microcontrolador **Raspberry Pi Pico W (RP2040)** utilizando seu **sensor de temperatura embutido**.

A leitura é realizada por meio do **conversor analógico-digital (ADC)** interno, e os valores obtidos são convertidos para **graus Celsius (°C)** utilizando a fórmula fornecida no **datasheet oficial do RP2040**.

## Funcionalidades

- Leitura do sensor de temperatura interno do MCU via **ADC**.
- Conversão do valor lido para **Celsius**.
- Exibição da temperatura no **monitor serial**, em tempo real.

## Fórmula Utilizada

A conversão da leitura ADC para temperatura em Celsius segue a fórmula do datasheet do RP2040:
    T(°C) = 27 - (ADC_voltage - 0.706) / 0.001721

> Onde:
> - `ADC_voltage` é o valor convertido da leitura do ADC para volts.

##  Aprendizados

Com esta atividade, foi possível compreender:

- O uso de sensores internos embarcados no RP2040.
- A aplicação prática de **conversão de sinais analógicos para digitais**.


## Materiais Utilizados

- **Placa BitDogLab**
- **Microcontrolador Raspberry Pi Pico W (RP2040)** MCU da placa

