#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include <string.h>
#include "hardware/pwm.h"

#include "hardware/clocks.h"


// Pino e canal do microfone no ADC.
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)

// Parâmetros e macros do ADC.
#define ADC_CLOCK_DIV 2000
#define ADC_SAMPLE_RATE  (48000000 / ADC_CLOCK_DIV)
#define SAMPLES ((int)(ADC_SAMPLE_RATE / 4)) // 0,25 segundos de amostras
#define ADC_ADJUST(x) (x * 3.3f / (1 << 12u) - 1.65f) // Ajuste do valor do ADC para Volts.
#define ADC_MAX 3.3f
#define ADC_STEP (3.3f/5.f) // Intervalos de volume do microfone.

//BUZZERS
#define BUZZER_A 21 // Pino para o buzzer A
#define PWM_WRAP 255

// LED RGB
#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13

//BOTÔES
#define BUTTON_A 5
#define BUTTON_B 6



#define RECORD_TIME 3 // Tempo de gravação em segundos
#define TOTAL_SAMPLES (RECORD_TIME * ADC_SAMPLE_RATE) // Total de amostras a serem gravadas
// Canal e configurações do DMA
uint dma_channel;
dma_channel_config dma_cfg;



uint16_t buffer[TOTAL_SAMPLES]; // Buffer A

volatile int posicao = 0; // Posição atual no buffer

enum Flags{
    FLAG_BUFFER = 1 << 0,
    FLAG_RECORD = 1 << 1,
    FLAG_DMA = 1 << 2,
    FLAG_TRATAMENTO = 1 << 3,
    FLAG_BUZZER = 1 << 4
};

volatile uint8_t status = 0;

static struct repeating_timer play_timer;

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;
int blocos_prontos = 0;

void init_record(); //captura os dados do adc e 
void init_adc();
void init_dma();
void init_rgb();
void init_buttons();
void gpio_callback(uint gpio, uint32_t events);
int64_t callback_timer(alarm_id_t id, void *user_data);
void dma_handler(); 
void init_pwm_buzzers();
bool play_callback(struct repeating_timer *t);


// fazer a exibicao da onda no display

int main()
{
    stdio_init_all();

    init_rgb();
    init_buttons();
    init_adc();
    init_dma();
    init_pwm_buzzers();
     

    while (true) {
        if((status & FLAG_RECORD) && !(status & FLAG_DMA)) {
            init_record();
            
        } 
       
        if(status & FLAG_BUZZER){

            gpio_put(LED_BLUE, true); // Acende o LED azul
            // Aciona o buzzer
            int delay_us = (int)(1e6 / ADC_SAMPLE_RATE);

            add_repeating_timer_us(delay_us, play_callback, NULL, &play_timer);
            status &= ~FLAG_BUZZER;

        }

           
    }
}

void init_adc(){
    //Configura o ADC
    adc_init();
    adc_gpio_init(MIC_PIN);
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
    // Tomando posse de um canal livre do DMA  
    dma_channel = dma_claim_unused_channel(true);
    // Pega a configuração padrão e ajusta apenas o essencial  
    dma_cfg = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);   // 16 bits por amostra  
    channel_config_set_read_increment(&dma_cfg, false);             // não incrementa na leitura (ADC FIFO)  
    channel_config_set_write_increment(&dma_cfg, true);             // incrementa o ponteiro de escrita (buffer)  
    channel_config_set_dreq(&dma_cfg, DREQ_ADC);                    // sincroniza com o ADC  

    // Habilita IRQ 
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void init_record() {
    
    adc_fifo_drain(); // Limpa o FIFO do ADC.
    adc_run(false); // Desliga o ADC (se estiver ligado) para configurar o DMA.

    status |= FLAG_DMA;
    dma_hw->ints0 = 1u << dma_channel;       // limpa IRQ
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_configure(dma_channel, &dma_cfg,
        buffer, // Escreve no buffer.
        &(adc_hw->fifo), // Lê do ADC.
        SAMPLES, // Faz "SAMPLES" amostras.
        false // Liga o DMA.
    );

    adc_run(true);
    dma_channel_start(dma_channel); // Inicia o DMA.

}

void gpio_callback(uint gpio, uint32_t events) {

    if(!(status & FLAG_RECORD)){
        gpio_put(LED_RED,true);
        gpio_put(LED_BLUE, false); // Apaga o LED azul
        add_alarm_in_ms(RECORD_TIME*1000, callback_timer, NULL, false);
        status |= FLAG_RECORD;    
    }
    
}

int64_t callback_timer(alarm_id_t id, void *user_data) {
    
    status &= ~FLAG_RECORD;
    status &= ~FLAG_DMA; 
    gpio_put(LED_RED,false);
    irq_set_enabled(DMA_IRQ_0, false);  // desativa a interrupção
    dma_channel_abort(dma_channel); // aborta o DMA
    dma_hw->ints0 = 1u << dma_channel;  // limpa a flag de interrupção pendente
    adc_run(false);
    posicao = 0; // Reseta a posição do buffer
    blocos_prontos = 0; // Reseta o contador de blocos prontos
    
    
    status |= FLAG_BUZZER;
    
    return 0; // 0 = não repetir
}

void dma_handler() { // AJuste manual economizando ciclos
    blocos_prontos++;
    dma_hw->ints0 = 1u << dma_channel;       // limpa IRQ
    posicao += SAMPLES;                // avanço no buffer
    // ajusta registros: end de leitura, end de escrita, contagem, e re-ativa
    dma_hw->ch[dma_channel].read_addr    = (uintptr_t)&adc_hw->fifo;
    dma_hw->ch[dma_channel].write_addr   = (uintptr_t)(buffer + posicao);
    dma_hw->ch[dma_channel].transfer_count = SAMPLES;
    dma_hw->ch[dma_channel].ctrl_trig   |= DMA_CH0_CTRL_TRIG_EN_BITS;
    status |= FLAG_BUFFER;
}


void init_pwm_buzzers(){
    
    // Inicializa os pinos dos buzzers
    gpio_set_function(BUZZER_A, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_A);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, PWM_WRAP); // Define o valor máximo do contador
   
    float clkdiv = (float)clock_get_hz(clk_sys) / (ADC_SAMPLE_RATE * (PWM_WRAP + 1));
   
    pwm_config_set_clkdiv(&config, clkdiv); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_A, 0); // Desliga o PWM inicialmente
}

bool play_callback(struct repeating_timer *t) {
    static int16_t y_last = 0;
    const float alpha = 0.3f;  // quanto menor, mais suavização
    static int idx = 0;
    if (idx >= TOTAL_SAMPLES) {
        gpio_put(LED_BLUE, false); // Apaga o LED azul
        pwm_set_gpio_level(BUZZER_A, 0);      // retoma silêncio
        idx = 0;
        memset(buffer, 0, sizeof(buffer)); // limpa o buffer
        idx=0;
        y_last=0;

        return false;                          // para o timer
    }
    // leu 0…4095
   
    
    int16_t centered = (int16_t)buffer[idx++] - 2048;
    int32_t amplified = centered * 8;
    if      (amplified >  2047) amplified =  2047;
    else if (amplified < -2048) amplified = -2048;
    // filtro IIR y[n] = α·x[n] + (1–α)·y[n–1]
    int16_t y = (int16_t)(alpha * amplified + (1.0f - alpha) * y_last);
    y_last = y;
    uint16_t pwm_val = (uint16_t)((y + 2048)>>4);  // de 0 a 255 para wrap=4095
    pwm_set_gpio_level(BUZZER_A, pwm_val); // Ajusta o PWM do buzzer A
    return true;                              // continua chamando a 24 kHz
}



