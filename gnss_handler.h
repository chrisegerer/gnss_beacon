#ifndef GNSS_HANDLER_H__
#define GNSS_HANDLER_H__

#include <stdint.h>

uint32_t gnss_handler_init(void);
bool gnss_handler_receive(uint8_t *received_bytes, uint8_t *buffer, uint8_t buffer_size);
void gnss_handler_transmit(const uint8_t *buffer, uint8_t buffer_size);

#endif // GNSS_HANDLER_H__