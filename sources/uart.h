// UART.

#ifndef _UART_H_
	#define _UART_H_

	#include <stdio.h>		// Стандартная библиотека ввода/вывода.

	void uart_init(uint8_t mode);
	char uart_get();
	void uart_put(uint8_t data);
	void uart_puts(char *s);
	void uart_puts_P(const char *progmem_s);

#endif