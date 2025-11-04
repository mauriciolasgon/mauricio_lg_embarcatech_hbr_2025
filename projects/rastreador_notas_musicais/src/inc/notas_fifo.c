#include "notas_fifo.h"
#include "pico/stdlib.h"


void fifo_init(fifo_t *fifo) {
    fifo->read_index = 0;
    fifo->write_index = 0;
    mutex_init(&fifo->mutex);
}

bool fifo_is_empty(fifo_t *fifo) {
    return fifo->read_index == fifo->write_index;
}

bool fifo_is_full(fifo_t *fifo) {
    return ((fifo->write_index + 1) % FIFO_SIZE) == fifo->read_index;
}

bool fifo_push(fifo_t *fifo, note_data item) {
    mutex_enter_blocking(&fifo->mutex);

    if (fifo_is_full(fifo)) {
        mutex_exit(&fifo->mutex);
        return false;
    }

    fifo->buffer[fifo->write_index] = item;
    fifo->write_index = (fifo->write_index + 1) % FIFO_SIZE;
    mutex_exit(&fifo->mutex);
    return true;
}

bool fifo_pop(fifo_t *fifo, note_data *item) {
    mutex_enter_blocking(&fifo->mutex);

    if (fifo_is_empty(fifo)) {
        mutex_exit(&fifo->mutex);
        return false;
    }
    *item = fifo->buffer[fifo->read_index];
    fifo->read_index = (fifo->read_index + 1) % FIFO_SIZE;

    mutex_exit(&fifo->mutex);
    return true;
}