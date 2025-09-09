#include <stdint.h>			// Коротние название int.
#include <avr/interrupt.h>	// Прерывания.

#include "uart.h"			// Свой заголовок.
#include "tcudata.h"		// Расчет и хранение всех необходимых параметров.

// Скорость передачи UART 57600 бит/с.
#define UART_BAUD_RATE 115200UL
// Значения регистров для настройки скорости UART.
// Вычисляется требуемое значение по формуле:
//	UBBR = F_CPU / (16 * baudrate) - 1		для U2X=0
//  UBBR = F_CPU / (8 * baudrate) - 1		для U2X=1

#define SET_UBRR ((F_CPU / (8UL * UART_BAUD_RATE)) - 1UL)

volatile uint16_t RxBuffPos = 0;					// Позиция в буфере.

uint8_t DataSize = sizeof(TCU) + 1;		// Размер пакета данных +1 байт типа данных.
// Признак, что был принят символ подмены байта.
volatile uint8_t MarkerByte = 0;

void uart_init(uint8_t mode) {
	// Сброс регистров настроек, так как загрузчик Arduino может нагадить.
	UCSR0A = 0;
	UCSR0B = 0;
	UCSR0C = 0;

	if (mode) {
		UCSR0A |= (1 << U2X0);							// Двойная скорость передачи.
		UCSR0C &= ~((1 << UMSEL01) | (1 << UMSEL00));	// Асинхронный режим.
		UCSR0C |= (1 << UCSZ00) | ( 1 << UCSZ01);		// Размер пакета 8 бит.
		UBRR0H = (uint8_t) (SET_UBRR >> 8);				// Настройка скорости.
		UBRR0L = (uint8_t) SET_UBRR;
	}
	else {
		// 0 - uart выключен.
		return;
	}
	
	switch (mode) {
		case 1:			
			UCSR0B |= (1 << RXEN0);			// 1 - Только прием.
			UCSR0B |= (1 << RXCIE0);		// Прерывание по завершеию приёма.
			break;
		case 2:
			UCSR0B |= (1 << TXEN0);		// 2 - Только передача.
			UCSR0B |= (1 << TXCIE0);	// Прерывание по завершеию передачи.
			break;
		case 3:
			// 3 - прием / передача.
			UCSR0B |= (1 << TXEN0);		// Прием.
			UCSR0B |= (1 << RXEN0);		// Передача.
			UCSR0B |= (1 << RXCIE0);		// Прерывание по завершеию приёма.
			UCSR0B |= (1 << TXCIE0);	// Прерывание по завершению передачи.
			break;
	}
}

// Отправить символ в UART
void uart_send_char(char Data) {
	// Ожидание готовности.
	while (!(UCSR0A & (1 << UDRE0))); 
	// Отправка.
	UDR0 = Data;                    
}

// Отправить строку в UART, используется только для отладки.
void uart_send_string(char* s) {
	// Пока строка не закончилась.
	while (*s) {
		// Отправить очередной символ.
		uart_send_char(*s);
		// Передвинуть указатель на следующий символ.
		s++;            
	}
}

// Прерывание по окончании приема.
ISR (USART_RX_vect) {
	uint8_t OneByte = UDR0;		// Получаем байт.

	if (DataStatus > 1) {return;}
	// Начальный адрес структуры TCU.
	uint8_t* TCUAddr = (uint8_t*) &TCU;

	switch (OneByte) {
		case FOBEGIN:	// Принят начальный байт.
			DataStatus = 1;
			RxBuffPos = 0;
			MarkerByte = 0;
			break;
		case FIOEND:	// Принят завершающий байт.
			if (RxBuffPos == DataSize) {DataStatus = 2;}
			else {DataStatus = 0;}
			break;
		case FESC:		// Принят символ подмены байта.
			MarkerByte = 1;
			break;
		default:
			if (RxBuffPos >= DataSize) {	// Пришло байт больше чем надо.
				DataStatus = 0;
				return;
			}

		 	if (MarkerByte) {	// Следующий байт после символа подмены.
		 		switch (OneByte) {
		 			case TFOBEGIN:
		 				OneByte = FOBEGIN;
		 				break;
		 			case TFIOEND:
		 				OneByte = FIOEND;
		 				break;
		 			case TFESC:
		 				OneByte = FESC;
		 				break;
		 			default:		// Если ничего не совпало, значит косяк.
		 				DataStatus = 0;
		 				MarkerByte = 0;
		 				return;
		 		}
		 		MarkerByte = 0;
		 	}

 		 	if (RxBuffPos == 0) {
			 	if (OneByte != TCU_DATA_PACKET) {		// Не тот пакет.
					DataStatus = 0;
					return;
				}
				RxBuffPos++;
		 	}
			else {
				*(TCUAddr + RxBuffPos - 1) = OneByte;
				RxBuffPos++;
			}
			break;
	}
}