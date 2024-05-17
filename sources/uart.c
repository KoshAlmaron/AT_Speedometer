#include <avr/pgmspace.h>	// Работа с PROGMEM.

#include "uart.h"			// Свой заголовок.

// Скорость передачи UART 57600 бит/с.
#define UART_BAUD_RATE 115200UL
// Значения регистров для настройки скорости UART.
// Вычисляется требуемое значение по формуле:
//	UBBR = F_CPU / (16 * baudrate) - 1		для U2X=0
//  UBBR = F_CPU / (8 * baudrate) - 1		для U2X=1

#define SET_UBRR ((F_CPU / (8UL * UART_BAUD_RATE)) - 1UL)

void uart_init (uint8_t mode) {
	// Сброс регистров настроек, так как загрузчик Arduino может нагадить.
	UCSR0A = 0;
	UCSR0B = 0;
	UCSR0C = 0;

	if (mode) {
		// Двойная скорость передачи.
		UCSR0A |= (1 << U2X0);
		// Асинхронный режим.
		UCSR0C &= ~((1 << UMSEL01) | (1 << UMSEL00));
		// Размер пакета 8 бит.
		UCSR0C |= (1 << UCSZ00) | ( 1 << UCSZ01);
		// Настройка скорости.
		UBRR0H = (uint8_t) (SET_UBRR >> 8);
		UBRR0L = (uint8_t) SET_UBRR;
	}
	else {
		// 0 - uart выключен.
		return;
	}
	
	switch (mode) {
		case 1:
			// 1 - Только прием.
			UCSR0B |= (1 << RXEN0);
			// Включение прерывания по приёму.
			UCSR0B |= (1<<RXCIE0);
			break;
		case 2:
			// 2 - Только передача.
			UCSR0B |= (1 << TXEN0);
			break;
		case 3:
			// 3 - прием / передача.
			UCSR0B |= (1 << TXEN0); 
			UCSR0B |= (1 << RXEN0);
			break;
	}
}

// принять символ из UART
char uart_get () {
	// Ожидание приёма.
	while (!(UCSR0A & (1 << RXC0)));
	// Вернуть принятый байт.
	return UDR0;                    
}

// Отправить символ в UART
void uart_put (uint8_t data) {
	// Ожидание готовности.
    while (!(UCSR0A & (1 << UDRE0))); 
    // Отправка.
    UDR0 = data;                    
}

// Отправить строку в UART
void uart_puts (char *s) {
	// Пока строка не закончилась.
    while (*s) {
    	// Отправить очередной символ.
        uart_put(*s);
        // Передвинуть указатель на следующий символ.
        s++;            
    }
}

// Отправить строку из памяти программ в UART
void uart_puts_P (const char *progmem_s) {
	register char c;
	// Пока строка не закончилась.
	while ((c = pgm_read_byte(progmem_s++))) {
		// Отправить очередной символ.
		uart_put(c);                               
	}
}

