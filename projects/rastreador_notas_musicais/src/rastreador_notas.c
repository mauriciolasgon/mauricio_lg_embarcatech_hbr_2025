
// Include standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
// Include Pico libraries
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"
// Include hardware libraries
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
// Include protothreads
#include "pt_cornell_rp2040_v1_3.h"
// Include para conexão wifi- Protocolo TCP
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "inc/ssd1306.h"
// Include da FIFO e da struct notas
#include "inc/notas_fifo.h"

// Define Botao B
#define BOTAO_B 6

// MACROS USADOS PARA FFT

typedef signed int fix15 ;
#define multfix15(a,b) ((fix15)((((signed long long)(a))*((signed long long)(b)))>>15))
#define float2fix15(a) ((fix15)((a)*32768.0)) // 2^15
#define fix2float15(a) ((float)(a)/32768.0)
#define absfix15(a) abs(a) 
#define int2fix15(a) ((fix15)(a << 15))
#define fix2int15(a) ((int)(a >> 15))
#define char2fix15(a) (fix15)(((fix15)(a)) << 15)

/////////////////////////// CONFIGURAÇÃO ADC ////////////////////////////////
// ADC Channel and pin
#define ADC_CHAN 2
#define ADC_PIN 26
// Number of samples per FFT
#define NUM_SAMPLES 1024
// Number of samples per FFT, minus 1
#define NUM_SAMPLES_M_1 1023
// Length of short (16 bits) minus log2 number of samples (10)
#define SHIFT_AMOUNT 6
// Log2 number of samples
#define LOG2_NUM_SAMPLES 10
// Sample rate (Hz)
#define Fs 10000.0
// ADC clock rate
#define ADCCLK 48000000.0

// Configurações de Wi-Fi
#define WIFI_SSID "x" 
#define WIFI_PASSWORD "x"
// Configurações do servidor

#define SERVER_IP "xxx.xxx.xx.x"
#define SERVER_PORT 5000
#define JSON_BUFFER_SIZE 128
#define MAX_RECONNECT_ATTEMPTS 3


// DMA channels for sampling ADC
int sample_chan ;
int control_chan ;

// Max and min macros
#define max(a,b) ((a>b)?a:b)
#define min(a,b) ((a<b)?a:b)


const uint I2C_SDA = 14; // pinos para a conexão I2C do display OLED
const uint I2C_SCL = 15;

// 0.4 in fixed point (used for alpha max plus beta min)
fix15 zero_point_4 = float2fix15(0.4) ;

// Here's where we'll have the DMA channel put ADC samples
uint16_t sample_array[NUM_SAMPLES] ;
// And here's where we'll copy those samples for FFT calculation
fix15 fr[NUM_SAMPLES] ;
fix15 fi[NUM_SAMPLES] ;

// Sine table for the FFT calculation
fix15 Sinewave[NUM_SAMPLES]; 
// Hann window table for FFT calculation
fix15 window[NUM_SAMPLES]; 

// Pointer to address of start of sample buffer
uint16_t * sample_address_pointer = &sample_array[0] ;

uint8_t ssd[ssd1306_buffer_length];


fifo_t shared_buffer_cores;

volatile bool tcp_connected = false;
volatile bool wifi_connected = false;


struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

typedef struct {
    struct tcp_pcb *pcb;
    ip_addr_t server_addr;
    bool connected;
    uint8_t reconnect_attempts;
} TcpConnection;

TcpConnection conn;

// Forward declaration of tcp_connection_init
bool tcp_connection_init(TcpConnection *conn);

// Peforms an in-place FFT. For more information about how this
// algorithm works, please see https://vanhunteradams.com/FFT/FFT.html
void FFTfix(fix15 fr[], fix15 fi[]) {
    
    unsigned short m;   // one of the indices being swapped
    unsigned short mr ; // the other index being swapped (r for reversed)
    fix15 tr, ti ; // for temporary storage while swapping, and during iteration
    
    int i, j ; // indices being combined in Danielson-Lanczos part of the algorithm
    int L ;    // length of the FFT's being combined
    int k ;    // used for looking up trig values from sine table
    
    int istep ; // length of the FFT which results from combining two FFT's
    
    fix15 wr, wi ; // trigonometric values from lookup table
    fix15 qr, qi ; // temporary variables used during DL part of the algorithm
    
    //////////////////////////////////////////////////////////////////////////
    ////////////////////////// BIT REVERSAL //////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Bit reversal code below based on that found here: 
    // https://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious
    for (m=1; m<NUM_SAMPLES_M_1; m++) {
        // swap odd and even bits
        mr = ((m >> 1) & 0x5555) | ((m & 0x5555) << 1);
        // swap consecutive pairs
        mr = ((mr >> 2) & 0x3333) | ((mr & 0x3333) << 2);
        // swap nibbles ... 
        mr = ((mr >> 4) & 0x0F0F) | ((mr & 0x0F0F) << 4);
        // swap bytes
        mr = ((mr >> 8) & 0x00FF) | ((mr & 0x00FF) << 8);
        // shift down mr
        mr >>= SHIFT_AMOUNT ;
        // don't swap that which has already been swapped
        if (mr<=m) continue ;
        // swap the bit-reveresed indices
        tr = fr[m] ;
        fr[m] = fr[mr] ;
        fr[mr] = tr ;
        ti = fi[m] ;
        fi[m] = fi[mr] ;
        fi[mr] = ti ;
    }
    //////////////////////////////////////////////////////////////////////////
    ////////////////////////// Danielson-Lanczos //////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Adapted from code by:
    // Tom Roberts 11/8/89 and Malcolm Slaney 12/15/94 malcolm@interval.com
    // Length of the FFT's being combined (starts at 1)
    L = 1 ;
    // Log2 of number of samples, minus 1
    k = LOG2_NUM_SAMPLES - 1 ;
    // While the length of the FFT's being combined is less than the number 
    // of gathered samples . . .
    while (L < NUM_SAMPLES) {
        // Determine the length of the FFT which will result from combining two FFT's
        istep = L<<1 ;
        // For each element in the FFT's that are being combined . . .
        for (m=0; m<L; ++m) { 
            // Lookup the trig values for that element
            j = m << k ;                         // index of the sine table
            wr =  Sinewave[j + NUM_SAMPLES/4] ; // cos(2pi m/N)
            wi = -Sinewave[j] ;                 // sin(2pi m/N)
            wr >>= 1 ;                          // divide by two
            wi >>= 1 ;                          // divide by two
            // i gets the index of one of the FFT elements being combined
            for (i=m; i<NUM_SAMPLES; i+=istep) {
                // j gets the index of the FFT element being combined with i
                j = i + L ;
                // compute the trig terms (bottom half of the above matrix)
                tr = multfix15(wr, fr[j]) - multfix15(wi, fi[j]) ;
                ti = multfix15(wr, fi[j]) + multfix15(wi, fr[j]) ;
                // divide ith index elements by two (top half of above matrix)
                qr = fr[i]>>1 ;
                qi = fi[i]>>1 ;
                // compute the new values at each index
                fr[j] = qr - tr ;
                fi[j] = qi - ti ;
                fr[i] = qr + tr ;
                fi[i] = qi + ti ;
            }    
        }
        --k ;
        L = istep ;
    }
}

///////// CAMADA DE INTERFACE COM O USUÁRIO //////////
//função para debounce do botão
bool debounce(int pin) {
    static volatile bool ultimo_estado_botao = false;

    bool estado_atual_botao = !gpio_get(pin);
    static absolute_time_t ultimo_tempo_botao = 0;
    if ( estado_atual_botao && !(ultimo_estado_botao) && absolute_time_diff_us(ultimo_estado_botao, get_absolute_time()) > 200000) {
        ultimo_tempo_botao = get_absolute_time();
        ultimo_estado_botao = true; 
        return true;
    }
     else if (!estado_atual_botao) {
        
        ultimo_estado_botao = false;
    }

    return false;
}
void print_Oled(uint8_t *ssd,char * string,struct render_area *frame_area){
    
    static int x=0, y=0;
    if (y>=64){
        y=0;
        //zera display
        memset(ssd, 0, ssd1306_buffer_length);
        render_on_display(ssd, frame_area);   
    }
    ssd1306_draw_string(ssd, x, y, string);
    render_on_display(ssd, frame_area);
    y+=8;

    
}


// Função para conectar ao Wi-Fi
bool connect_to_wifi() {
    printf("Conectando ao Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 40000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return false;
    }
    printf("Conectado ao Wi-Fi\n");
    
    
    return true;
}


void disconnect_wifi() {
    printf("Desconectando do Wi-Fi...\n");
    cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
    wifi_connected = false;
    printf("Desconectado do Wi-Fi!\n");
    sleep_ms(2000);
}


void serialize_note_data(note_data *data, char *buffer, int buffer_size) {
    snprintf(buffer, buffer_size,
             "{\"nota\": \"%s\", \"freq\": %.2f, \"oitava\": %d, \"cents\": %.2f}",
             data->nota, data->freq, data->oitava, data->desvio);
}

bool tcp_reconnect(TcpConnection *conn) {
    if (conn->connected) {
        return true; // Já está conectado
    }

    printf("Tentando reconexão...\n");
    
    // Tenta reinicializar a conexão
    if (tcp_connection_init(conn)) {

        return true; // Tentativa de reconexão iniciada
    }
    
    printf("Falha ao iniciar reconexão.\n");
    return false;
}

void tcp_cleanup(TcpConnection *conn) {
    if (conn->pcb) {
        tcp_close(conn->pcb);
        conn->pcb = NULL;
    }
    conn->connected = false;
}

err_t tcp_connection_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TcpConnection *conn = (TcpConnection *)arg;

    if (err != ERR_OK) {
        printf("Erro na conexão (%d).\n", err);
        tcp_cleanup(conn);
        return err;
    }

    printf("Conexão estabelecida com %s:%d\n", ip4addr_ntoa(&conn->server_addr), SERVER_PORT);
    conn->connected = true;
    return ERR_OK;
}

bool tcp_connection_init(TcpConnection *conn) {
    memset(conn, 0, sizeof(TcpConnection));

    // Configura endereço do servidor
    if (!ip4addr_aton(SERVER_IP, &conn->server_addr)) {
        printf("Endereço IP inválido\n");
        return false;
    }

    conn->pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!conn->pcb) {
        printf("Falha ao criar PCB\n");
        return false;
    }

    tcp_arg(conn->pcb, conn);
    err_t err = tcp_connect(conn->pcb, &conn->server_addr, SERVER_PORT, tcp_connection_callback);
    
    if (err != ERR_OK) {
        printf("Erro de conexão inicial: %d\n", err);
        tcp_cleanup(conn);
        return false;
    }

    return true;
}

bool tcp_send_data(TcpConnection *conn, note_data *nota) {
    if (!conn->connected) {
        return false;
    }

    char json_data[JSON_BUFFER_SIZE];
    serialize_note_data(nota, json_data, sizeof(json_data));

    if (tcp_sndbuf(conn->pcb) < strlen(json_data)) {
        printf("Buffer de envio cheio. Aguardando...\n");
        return false;
    }

    err_t err = tcp_write(conn->pcb, json_data, strlen(json_data), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Erro no envio: %d\n", err);
        tcp_cleanup(conn);
        return false;
    }

    if (tcp_output(conn->pcb) != ERR_OK) {
        printf("Erro ao enviar dados\n");
        tcp_cleanup(conn);
        return false;
    }

    return true;
}

void init_i2c(){
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}



void config_oled(uint8_t* ssd){
    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}




note_data note_detection(float freq){

    static note_data nota = {0};
    nota.freq = freq;
    
    const char *notas[]={ "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

    float midi_number = 12 * log2f(freq / 440.0) + 69;
    int midi_rounded = (int)roundf(midi_number);

    // Mapeia para o nome da nota e oitava
    int nota_index = midi_rounded % 12;
    nota.nota = notas[nota_index];
    int oitava = (midi_rounded / 12) - 1; // Oitava 4 é a central
    nota.oitava=oitava;

    // Calcula a frequência de referência para a nota ideal
    float freq_referencia = 440.0 * powf(2.0, (midi_rounded - 69) / 12.0);

    // Calcula os "cents" (desvio em relação à nota ideal)
    float cents = 1200 * log2f(freq / freq_referencia);
    nota.desvio = cents;
    return nota;
}
// Runs on core 0
static PT_THREAD (protothread_fft(struct pt *pt))
{
    // Indicate beginning of thread
    PT_BEGIN(pt) ;
    printf("Starting capture\n") ;
    // Start the ADC channel
    dma_start_channel_mask((1u << sample_chan)) ;
    // Start the ADC
    adc_run(true) ;

    // Declare some static variables
    static int height ;             // for scaling display
    static float max_freqency ;     // holds max frequency
    static int i ;                  // incrementing loop variable

    static fix15 max_fr ;           // temporary variable for max freq calculation
    static int max_fr_dex ;         // index of max frequency




    while(1) {
        // Wait for NUM_SAMPLES samples to be gathered
        // Measure wait time with timer. THIS IS BLOCKING
        dma_channel_wait_for_finish_blocking(sample_chan);

        // Copy/window elements into a fixed-point array
        for (i=0; i<NUM_SAMPLES; i++) {
            fr[i] = multfix15(int2fix15((int)sample_array[i]), window[i]) ;
            fi[i] = (fix15) 0 ;
        }

        // Zero max frequency and max frequency index
        max_fr = 0 ;
        max_fr_dex = 0 ;

        // Restart the sample channel, now that we have our copy of the samples
        dma_channel_start(control_chan) ;

        // Compute the FFT
        FFTfix(fr, fi) ;

        // Find the magnitudes (alpha max plus beta min)
        for (int i = 0; i < (NUM_SAMPLES>>1); i++) {  
            // get the approx magnitude
            fr[i] = abs(fr[i]); 
            fi[i] = abs(fi[i]);
            // reuse fr to hold magnitude
            fr[i] = max(fr[i], fi[i]) + 
                    multfix15(min(fr[i], fi[i]), zero_point_4); 

            // Keep track of maximum
            if (fr[i] > max_fr && i>4) {
                max_fr = fr[i] ;
                max_fr_dex = i ;
            }
        }
        // Compute max frequency in Hz
        max_freqency = max_fr_dex * (Fs/NUM_SAMPLES) ;
        
        note_data nota = note_detection(max_freqency);
        
        fifo_push(&shared_buffer_cores,nota);
        printf(" %d\n", nota.freq);


    }
    PT_END(pt) ;
}

//Thread roda no core1
static PT_THREAD (protothread_blink(struct pt *pt))
{
    // Indicate beginning of thread
    PT_BEGIN(pt) ;
    
    note_data nota;
    while (1) {

        if(!wifi_connected){
            if(debounce(BOTAO_B)){
                print_Oled(ssd,"Conectando....",&frame_area);
                wifi_connected=connect_to_wifi();
                if(wifi_connected){
                    print_Oled(ssd,"Conectado ao Wi-Fi",&frame_area);
                }else{
                    print_Oled(ssd,"Falha ao conectar ao Wi-Fi",&frame_area);
                }
                sleep_ms(2000);
            }
        }else{
            if(debounce(BOTAO_B)){
                disconnect_wifi();
                print_Oled(ssd,"Desconectado do Wi-Fi",&frame_area);
                sleep_ms(2000);
            }
            cyw43_arch_poll();
        }
        
        
        if(fifo_pop(&shared_buffer_cores,&nota)){
            tcp_send_data(&conn, &nota);
            
        }else{
            printf("Fila Vazia\n");
            
        }
        
        if(!tcp_connected){
            tcp_reconnect(&conn);
        }

        
        
        PT_YIELD_usec(100000) ;
    }
    PT_END(pt) ;
}

// Core 1 entry point (main() for core 1)
void core1_entry() {
    // Add and schedule threads
    pt_add_thread(protothread_blink) ;
    pt_schedule_start ;
}

// Core 0 entry point
int main() {
    // Initialize stdio
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("Falha ao inicializar o Wi-Fi\n");
    }
    cyw43_arch_enable_sta_mode();
    wifi_connected=connect_to_wifi();

    init_i2c();
    ssd1306_init();
    fifo_init(&shared_buffer_cores);
    config_oled(ssd);
    sleep_ms(2000) ;
    if (!tcp_connection_init(&conn)) {
        printf("Falha inicial na conexão\n");
    }
    
    

    if(wifi_connected){
        print_Oled(ssd,"Conectado ao Wi-Fi",&frame_area);
    }else{
        print_Oled(ssd,"Falha ao conectar ao Wi-Fi",&frame_area);
    }


    //Inicialização do botão B
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);


    ///////////////////////////////////////////////////////////////////////////////
    // ============================== ADC CONFIGURATION ==========================
    //////////////////////////////////////////////////////////////////////////////
    // Init GPIO for analogue use: hi-Z, no pulls, disable digital input buffer.
    adc_gpio_init(ADC_PIN);

    // Initialize the ADC harware
    // (resets it, enables the clock, spins until the hardware is ready)
    adc_init() ;

    // Select analog mux input (0...3 are GPIO 26, 27, 28, 29; 4 is temp sensor)
    adc_select_input(ADC_CHAN) ;

    // Setup the FIFO
    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        false     // Shift each sample to 8 bits when pushing to FIFO
    );

   
    adc_set_clkdiv(ADCCLK/Fs);


    // Populate the sine table and Hann window table
    int ii;
    for (ii = 0; ii < NUM_SAMPLES; ii++) {
        Sinewave[ii] = float2fix15(sin(6.283 * ((float) ii) / (float)NUM_SAMPLES));
        window[ii] = float2fix15(0.5 * (1.0 - cos(6.283 * ((float) ii) / ((float)NUM_SAMPLES))));
    }

    /////////////////////////////////////////////////////////////////////////////////
    // ============================== ADC DMA CONFIGURATION =========================
    /////////////////////////////////////////////////////////////////////////////////

    sample_chan = dma_claim_unused_channel(true);
    control_chan = dma_claim_unused_channel(true);

    // Channel configurations
    dma_channel_config c2 = dma_channel_get_default_config(sample_chan);
    dma_channel_config c3 = dma_channel_get_default_config(control_chan);


    // ADC SAMPLE CHANNEL
    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&c2, DMA_SIZE_16);
    channel_config_set_read_increment(&c2, false);
    channel_config_set_write_increment(&c2, true);
    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&c2, DREQ_ADC);
    // Configure the channel
    dma_channel_configure(sample_chan,
        &c2,            // channel config
        sample_array,   // dst
        &adc_hw->fifo,  // src
        NUM_SAMPLES,    // transfer count
        false            // don't start immediately
    );

    // CONTROL CHANNEL
    channel_config_set_transfer_data_size(&c3, DMA_SIZE_32);      // 32-bit txfers
    channel_config_set_read_increment(&c3, false);                // no read incrementing
    channel_config_set_write_increment(&c3, false);               // no write incrementing
    channel_config_set_chain_to(&c3, sample_chan);                // chain to sample chan

    dma_channel_configure(
        control_chan,                         // Channel to be configured
        &c3,                                // The configuration we just created
        &dma_hw->ch[sample_chan].write_addr,  // Write address (channel 0 read address)
        &sample_address_pointer,                   // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    print_Oled(ssd,"Inicializando",&frame_area);
    // Launch core 1
    multicore_launch_core1(core1_entry);



    // Add and schedule core 0 threads
    pt_add_thread(protothread_fft) ;
    pt_schedule_start ;

}