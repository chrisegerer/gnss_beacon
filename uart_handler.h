#ifndef UART_HANDLER_H__
#define UART_HANDLER_H__

#include <stdint.h>

uint32_t uart_handler_init(void);
bool uart_handler_receive(uint8_t *received_bytes, uint8_t *buffer, uint8_t buffer_size);
void uart_handler_transmit(const uint8_t *buffer, uint8_t buffer_size);

#endif // UART_HANDLER_H__