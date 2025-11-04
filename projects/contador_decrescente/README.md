# Contador Decrescente com Interrupções

##  Descrição da Atividade

Esta atividade consiste no desenvolvimento de um programa embarcado que realiza a contagem de pressionamentos do **botão B** durante um intervalo de tempo pré-definido de **9 segundos**. Durante esse período, o tempo restante é exibido de forma decrescente.

- O **botão A** é responsável por **reiniciar o cronômetro** e a contagem de pressionamentos.
- A **interrupção via GPIO** foi utilizada para ambos os botões, permitindo que o programa principal se concentre na **atualização do display OLED** e no controle do tempo, sem a necessidade de monitorar constantemente os botões.

##  Funcionalidades

-  Exibição do tempo restante e da contagem do botão B no **display OLED**.
-  Contagem dos pressionamentos do botão B durante os 9 segundos.
-  Reinício da contagem e do tempo ao pressionar o botão A.

##  Aprendizados

Esta atividade não só **reforçou o conceito de interrupções** e seus benefícios em sistemas embarcados, como também evidenciou problemas comuns, como o **efeito de bouncing**, que pode causar a leitura de múltiplos cliques para uma única pressão do botão. Esse comportamento pode ser mitigado com técnicas de debounce por software ou hardware.

##  Materiais Utilizados

-  **Placa BitDogLab**
-  **2 botões**
-  **1 display OLED**
