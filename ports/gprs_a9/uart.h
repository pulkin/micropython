#include <stdint.h>
#include <stdio.h>

#include "py/ringbuf.h"

#include "api_hal_uart.h"

#define UART_NPORTS (2)
#define UART_STATIC_RXBUF_LEN (256)

void uart_set_rxbuf(uint8_t uart, uint8_t *buf, int len);
int uart_get_rxbuf_len(uint8_t uart);
bool uart_rx_wait(uint8_t uart, uint32_t timeout_us);
int uart_rx_char(uint8_t uart);
uint32_t uart_tx_one_char(uint8_t uart, char c);
int uart_rx_any(uint8_t uart);
int uart_tx_any_room(uint8_t uart);
bool uart_setup(uint8_t uart);
bool uart_close(uint8_t uart);

extern uint8_t *uart_ringbuf_array[2];
extern UART_Config_t uart_dev[UART_NPORTS];
extern ringbuf_t uart_ringbuf[UART_NPORTS];

