#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

// Pino e canal do microfone no ADC.
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)

// Parâmetros e macros do ADC.
#define ADC_CLOCK_DIV 96.f
#define ADC_SAMPLE_RATE  48.000.000f / ADC_CLOCK_DIV
#define SAMPLES 200 // Número de amostras que serão feitas do ADC.
#define ADC_ADJUST(x) (x * 3.3f / (1 << 12u) - 1.65f) // Ajuste do valor do ADC para Volts.
#define ADC_MAX 3.3f
#define ADC_STEP (3.3f/5.f) // Intervalos de volume do microfone.

// LED RGB
#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13

//BOTÔES
#define BUTTON_A 5
#define BUTTON_B 6

// Canal e configurações do DMA
uint dma_channel;
dma_channel_config dma_cfg;

// Buffer de amostras do ADC.
uint16_t adc_buffer[SAMPLES];

// tempo de gravação(ms)
int record_time=10000;
bool record=false;
bool buffer_pronto=false;

enum Flags{
    FLAG_BUFFER = 1 << 0,
    FLAG_RECORD = 1 << 1,
    FLAG_DMA = 1 << 2
};

uint8_t status = 0;

void init_record(); //captura os dados do adc e 
void init_adc();
void init_dma();
void init_rgb();
void init_buttons();
void gpio_callback(uint gpio, uint32_t events);
int64_t callback_timer(alarm_id_t id, void *user_data);
void dma_handler(); 


// Arrumar gerenciamento do dma e adc apos o fim da gravacao
// fazer a exibicao da onda
// iniciar construcao pwm


int main()
{
    stdio_init_all();

    init_rgb();
    init_buttons();
    init_adc();
    init_dma();

    while (true) {
        if((status & FLAG_RECORD) && !(status & FLAG_DMA)){
            init_record();
        }
        // enquanto grava processa os dados
        if(status & FLAG_BUFFER){
            status &= ~FLAG_BUFFER;
            //processa os dados com 200 samples
        }
        
    }
}

void init_adc(){
    //Configura o ADC
    adc_gpio_init(MIC_PIN);
    adc_init();
    adc_select_input(MIC_CHANNEL);


    adc_fifo_setup(
        true, // Habilitar FIFO
        true, // Habilitar request de dados do DMA
        1, // Threshold para ativar request DMA é 1 leitura do ADC
        false, // Não usar bit de erro
        false // Não fazer downscale das amostras para 8-bits, manter 12-bits.
    );

    adc_set_clkdiv(ADC_CLOCK_DIV);
}

void init_buttons(){
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    //Configura a interrupcao do botao A para o timer
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
}

void init_rgb(){
    //Inicialização do LED RGB
    gpio_init(LED_BLUE);
    gpio_init(LED_GREEN);
    gpio_init(LED_RED);
    gpio_set_dir(LED_BLUE,GPIO_OUT);
    gpio_set_dir(LED_GREEN,GPIO_OUT);
    gpio_set_dir(LED_RED,GPIO_OUT);
}

void init_dma(){
    // Tomando posse de canal do DMA.
    dma_channel = dma_claim_unused_channel(true);
    // Configurações do DMA.
    dma_cfg = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16); // Tamanho da transferência é 16-bits (usamos uint16_t para armazenar valores do ADC)
    channel_config_set_read_increment(&dma_cfg, false); // Desabilita incremento do ponteiro de leitura (lemos de um único registrador)
    channel_config_set_write_increment(&dma_cfg, true); // Habilita incremento do ponteiro de escrita (escrevemos em um array/buffer)
    channel_config_set_dreq(&dma_cfg, DREQ_ADC); // Usamos a requisição de dados do ADC
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

}

void init_record() {
    adc_fifo_drain(); // Limpa o FIFO do ADC.
    adc_run(false); // Desliga o ADC (se estiver ligado) para configurar o DMA.

    status |= FLAG_DMA;
    dma_channel_configure(dma_channel, &dma_cfg,
        adc_buffer, // Escreve no buffer.
        &(adc_hw->fifo), // Lê do ADC.
        SAMPLES, // Faz "SAMPLES" amostras.
        true // Liga o DMA.
    );

    // Liga o ADC e espera acabar a leitura.
    adc_run(true);

}

void gpio_callback(uint gpio, uint32_t events) {
    // Inicia o timer por 5 segundos
    if(!(status & FLAG_RECORD)){
        gpio_put(LED_RED,true);
        add_alarm_in_ms(record_time, callback_timer, NULL, false);
        status |= FLAG_RECORD;
    }
}

int64_t callback_timer(alarm_id_t id, void *user_data) {
    // Código a ser executado após 5 segundos
    // Isso atua como a "interrupção do timer"
    status &= ~FLAG_RECORD;
    status &= ~FLAG_DMA; 
    gpio_put(LED_RED,false);
    adc_run(false);
    dma_channel_abort(dma_channel);

    //verificar se há dados para processar

    printf("Tempo esgotado!\n");
    return 0; // 0 = não repetir
}

void dma_handler() {
    // Verifica se a interrupção veio do canal específico
    if (dma_hw->ints0 & (1u << dma_channel)) {
        dma_hw->ints0 = 1u << dma_channel; // limpa a flag de interrupção

        // Aqui entra sua lógica: processar, copiar, iniciar nova transferência...
        status |= FLAG_BUFFER;
    }
}



