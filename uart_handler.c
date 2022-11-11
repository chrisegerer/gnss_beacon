#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "uart_handler.h"
#include "bsp.h"
#include "nrf_uarte.h"
#include "app_uart.h"
#include "app_error.h"

#define UART_RX_BUF_SIZE 256
#define UART_TX_BUF_SIZE 256
#define UART_NO_PARITY false

#define DATA_BUFFER_SIZE 32U
#define CR  '\r'
#define LF  '\n'
#define EOL '\0'

static bool received_new_line;
static uint8_t data_buffer[DATA_BUFFER_SIZE];

static void uart_error_handle(app_uart_evt_t * p_event);

uint32_t uart_handler_init(void)
{
    uint32_t err_code = NRF_SUCCESS;

    const app_uart_comm_params_t comm_params =
    {
        RX_PIN_NUMBER,
        TX_PIN_NUMBER,
        RTS_PIN_NUMBER,
        CTS_PIN_NUMBER,
        APP_UART_FLOW_CONTROL_DISABLED,
        UART_NO_PARITY,
        NRF_UARTE_BAUDRATE_115200
    };

    received_new_line = false;
    memset(data_buffer, 0U, DATA_BUFFER_SIZE);

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_error_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);

    APP_ERROR_CHECK(err_code);

    return err_code;
}

bool uart_handler_receive(uint8_t * idx, uint8_t * buffer, uint8_t buffer_size)
{
    bool new_location_received = false;
    uint8_t c;

    if ((NULL != buffer) && (buffer_size > 0U))
    {
        while(NRF_SUCCESS == app_uart_get(&c))
        {
            if ((CR == c) || (LF == c))
            {
                new_location_received = true;
            }
            else if (*idx < buffer_size)
            {
                buffer[*idx] = c;
                ++(*idx);
            }
        }
    }

    // remove any trailing CR or LF
    if (*idx >= 2U)
    {
        if ((CR == buffer[*idx - 2U]) || (LF == buffer[*idx - 2U]))
        {
            *idx -= 2U;
        }
        else if ((CR == buffer[*idx - 1U]) || (LF == buffer[*idx - 1U]))
        {
            --(*idx);
        }
    }

    return new_location_received;
}

void uart_handler_transmit(const uint8_t * buffer, uint8_t buffer_size)
{
    if ((NULL != buffer) && (buffer_size > 0U))
    {
        for (uint8_t idx = 0U; idx < buffer_size; ++idx)
        {
            app_uart_put(buffer[idx]);
        }
        app_uart_put(CR);
        app_uart_put(LF);
    }
}

static void uart_error_handle(app_uart_evt_t * p_event)
{
    switch(p_event->evt_type)
    {
    case APP_UART_COMMUNICATION_ERROR:
        APP_ERROR_HANDLER(p_event->data.error_communication);
        break;

    case APP_UART_FIFO_ERROR:
        APP_ERROR_HANDLER(p_event->data.error_code);
        break;
    
    default:
        break;
    }
}