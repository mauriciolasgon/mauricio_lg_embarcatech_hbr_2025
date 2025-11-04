#ifndef NOTAS_FIFO_H

#define NOTAS_FIFO_H
    #include "pico/mutex.h"
    #define FIFO_SIZE 20

    typedef struct note_data{
        const char* nota;
        float freq;
        int oitava;
        float desvio;
    }note_data;

    typedef struct {
        note_data buffer[FIFO_SIZE];
        int read_index; // inicio da fila
        int write_index; //final da fila
        mutex_t mutex;
    } fifo_t;

    
    // Inicializa a FIFO
    void fifo_init(fifo_t *fifo);

    // Verifica se está vazia
    bool fifo_is_empty(fifo_t *fifo);

    // Verifica se está cheia
    bool fifo_is_full(fifo_t *fifo);

    // Adiciona um item (escrita)
    bool fifo_push(fifo_t *fifo, note_data item);

    // Remove um item (leitura)
    bool fifo_pop(fifo_t *fifo, note_data *item);
#endif // NOTAS_FIFO_H