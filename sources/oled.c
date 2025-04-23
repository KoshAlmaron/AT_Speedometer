#include <stdint.h>			// Коротние название int.
#include "oled.h"			// Свой заголовок.
#include "i2c.h"			// I2C (TWI).
#include <avr/pgmspace.h>	// Работа с PROGMEM.

#include "uart.h"			// UART.
/***********!!! НЕЛЬЗЯ ТРОГАТЬ БУФЕР ПОКА ОН НЕ ОТПРАВЛЕН !!!************/

#define OLED_WIDTH	128
#define OLED_HEIGHT	32

#define DATA_BUFFER_SIZE OLED_WIDTH * OLED_HEIGHT / 8
#define SEND_BUFFER_SIZE DATA_BUFFER_SIZE + 14
#define COMMAND_BUFFER_SIZE 10

// Массив данных экрана (128 * 32 / 8 + адрес + команда). 
uint8_t SendBuffer[SEND_BUFFER_SIZE];
// Указатель на третий элемент массива - первый элемент данных.
uint8_t *DataBuffer = (SendBuffer + 14);

// Массив для отправки команд.
uint8_t CommandBuffer[COMMAND_BUFFER_SIZE];

// Режим отрисовки пикселя, 0 - номальный, 1 - инверсия (XOR), 2 - стирание.
uint8_t DrawMode = 0;

// Ограничения для отрисовки пикселей.
uint8_t Xmin = 0;
uint8_t Ymin = 0;
uint8_t Xmax = OLED_WIDTH - 1;
uint8_t Ymax = OLED_HEIGHT - 1;

// Параметры выбранного шрифта.
const uint8_t* Font = 0;		// Указатель на массив символов.
uint8_t FontFirstSymbol = 0;	// Код первого символа в массиве.
uint8_t FontSymbolsCount = 0;	// Количество символов.
uint8_t FontHeight = 0;			// Высота шрифта.
uint8_t FontSpace = 0;			// Расстояние между символами.

// Настройка дисплея, необходимо передать адрес и тип.
void oled_init(uint8_t Addr, uint8_t Contrast, uint8_t Flip) {
	SendBuffer[0] = Addr;		// Первый элемент это адрес устройства.

	// Используем буфер данных для отправки команд инициализации.	
	for (uint8_t i = 0; i < 26; i++ ) {
		SendBuffer[i * 2 + 1] = 0x80;	
	}
	SendBuffer[2] = 0xAE;	// Выключить дисплей.
	SendBuffer[4] = 0xD5;	// Установка делителя частоты,
	SendBuffer[6] = 	0x80;	// Значение делителя.
	SendBuffer[8] = 0xA8;	// Установка коэффициента мультиплексирования,
	SendBuffer[10] = 	0x3F;	// Значение параметра.
	SendBuffer[12] = 0xD3;	// Смещение дисплея,
	SendBuffer[14] = 	0x00;	// Значение - 0.
	SendBuffer[16] = 0x40;	// Начальная линия,
	SendBuffer[18] = 0x8D;	// Настройка charge pump,
	SendBuffer[20] = 	0x14;	// Разрешаем charge pump.
	SendBuffer[22] = 0x20;	// Режим адресации памяти,
	SendBuffer[24] = 	0x00; 	// Значение - горизонтальный.
	SendBuffer[26] = 0xA1; 	// Отображение по горизонтали, A1 - нормально, A0 - зеркально.
	SendBuffer[28] = 0xC8;	// Отображение по вертикали, С8 - нормально, С0 - зеркально.
	SendBuffer[30] = 0xDA;	// Конфигурацию выводов COM-сигналов.
	SendBuffer[32] = 	0x12;	// Значение параметра.	*** 128x32 - 12
	SendBuffer[34] = 0x81;	// Установка контрастности. 
	SendBuffer[36] = 	Contrast;	// Значение контрастности.
	SendBuffer[38] = 0xD9;	// Установки длительности периода pre-charge.
	SendBuffer[40] = 	0xF1;	// Значение параметра.
	SendBuffer[42] = 0xDB;	// Настраивает выход регулятора напряжения VCOMH.
	SendBuffer[44] = 	0x40;	// Значение параметра.
	SendBuffer[46] = 0x2E; 	// Отключение прокрутки.
	SendBuffer[48] = 0xA4; 	// Отображение содержимого RAM.
	SendBuffer[50] = 0xA6; 	// 0xA6 - нормальное отображение, 0xA7 - инверсное.
	SendBuffer[52] = 0xAF;	// 0xAF - экран включен в нормальном режиме, 0xAE - экран выключен.

	if (OLED_HEIGHT == 32) {
		SendBuffer[10] = 0x1F;	 // 128x32 - 0x1F.
		SendBuffer[32] = 0x02;	 // 128x32 - 0x02.
	}

	if (Flip) {
		SendBuffer[26] = 0xA0; 	// Отображение по горизонтали, A1 - нормально, A0 - зеркально.
		SendBuffer[28] = 0xC0;	// Отображение по вертикали, С8 - нормально, С0 - зеркально.		
	}

	while (!(i2c_ready()));				// Ожидание готовности интерфейса.
	i2c_send_array(SendBuffer, 53);		// Отправка.
	while (!(i2c_ready()));

	// Комманды, которые необходимо послать перед отправкой данных на экран.
	// Курсор переходит в нулевую позицию автоматом, но при потере
	// хотя бы одного байта при передаче произойдет смещение.
	// Потому лучше всегда устанавливать курсор в 0 перед передачей данных.

	// Команда 0x80 была записана ранее при инициализации.
	SendBuffer[2] = 0x21;	// Установка диапазона столбцов:
	SendBuffer[4] = 0;			// первый,
	SendBuffer[6] = 127;		// последний.
	SendBuffer[8] = 0x22;	// Установка диапазона страниц (строк):
	SendBuffer[10] = 0;			// первая,
	SendBuffer[12] = (OLED_HEIGHT >> 3) - 1;	// последняя.
	SendBuffer[13] = 0x40;	// Второй элемент комманда отправки массива данных.

	// Заготовка для буфера команд.
	CommandBuffer[0] = Addr;	// Первый элемент это адрес устройства.
	// Все нечетныне элементы 0x80 означает, что следующий байт - это команда.
	for (uint8_t i = 0; i < COMMAND_BUFFER_SIZE / 2; i++ ) {
		CommandBuffer[i * 2 + 1] = 0x80;	
	}

	// Очистка экрана.
	for (uint16_t i = 0; i < DATA_BUFFER_SIZE; i++) {
		DataBuffer[i] = 0;
	}
	while (!(i2c_ready()));
	i2c_send_array(SendBuffer, SEND_BUFFER_SIZE);
	while (!(i2c_ready()));
}

void oled_clear_buffer() {
	for (uint16_t i = 0; i < DATA_BUFFER_SIZE; i++) {
		DataBuffer[i] = 0;
	}
}

void oled_send_data() {
	i2c_send_array(SendBuffer, SEND_BUFFER_SIZE);
}

void oled_draw_mode(uint8_t Mode) {
	DrawMode = Mode;
}

void oled_set_clip_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
	Xmin = x0;
	Ymin = y0;
	Xmax = x1;
	Ymax = y1;
}

void oled_disable_clip_window() {
	Xmin = 0;
	Ymin = 0;
	Xmax = OLED_WIDTH - 1;
	Ymax = OLED_HEIGHT - 1;
}

void oled_draw_pixel(uint8_t x, uint8_t y) {
	if (x < Xmin || x > Xmax) {return;}
	if (y < Ymin || y > Ymax) {return;}

	uint16_t ByteNumber = ((y >> 3) << 7) + x;
	if (ByteNumber < DATA_BUFFER_SIZE) {
		switch (DrawMode) {			// Режим отрисовки пикселя.
			case 0:					// 0 - номальный.
				DataBuffer[ByteNumber] |= 1 << (y % 8);
				break;
			case 1:					// 1 - инверсия (XOR).
				DataBuffer[ByteNumber] ^= 1 << (y % 8);
				break;
			case 2:					// 2 - стирание.
				DataBuffer[ByteNumber] &= ~(1 << (y % 8));
				break;
		}
	}
}

void oled_draw_h_line(uint8_t x, uint8_t y, uint8_t l) {
	for (uint8_t i = 0; i < l; i++) {
		oled_draw_pixel(x + i, y);
	}
}

void oled_draw_v_line(uint8_t x, uint8_t y, uint8_t l) {
	for (uint8_t i = 0; i < l; i++) {
		oled_draw_pixel(x, y + i);
	}
}

void oled_draw_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
	for (uint8_t i = 0; i < h; i++) {
		oled_draw_h_line(x, y + i, w);
	}
}

void oled_draw_frame(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
	if (w > 1) {
		oled_draw_h_line(x, y, w);
		if (h > 1) {oled_draw_h_line(x, y + h - 1, w);}
	}
	if (h > 2) {
		oled_draw_v_line(x, y + 1, h - 2);
		if (w > 1) {oled_draw_v_line(x + w - 1, y + 1, h - 2);}
	}
}

void oled_draw_xbmp(uint8_t x, uint8_t y, const uint8_t* xbm, uint8_t w, uint8_t h) {
	uint8_t blen = (w + 7) >> 3;	// Ширина в байтах.
	uint8_t mask;					// Маска получения бита.

	for (uint8_t i = 0; i < h; i++) {
		for (uint8_t k = 0; k < blen; k++) {
			mask = 1;
			for (uint8_t j = 0; j < 8; j++) {
				uint8_t px = x + k * 8 + j;
				//if (px >= OLED_WIDTH) {break;}	// Достигнут предел экрана по X.
				if (px >= OLED_WIDTH) {px -= OLED_WIDTH;}
				if (pgm_read_byte(xbm + k) & mask) {oled_draw_pixel(px, y);}
				mask <<= 1;
			}
		}
		xbm += blen;					// Смещаем позицию в массиве на 1 строку.
		y++;							// Переход на следующуй строку.
		if (y >= OLED_HEIGHT) {return;}	// Достигнут предел экрана по Y.
	}
}

void oled_set_font(const uint8_t* FontName) {
	FontFirstSymbol = pgm_read_byte(FontName);
	FontSymbolsCount = pgm_read_byte(FontName + 1);
	FontHeight = pgm_read_byte(FontName + 2);
	FontSpace = pgm_read_byte(FontName + 3);
	Font = FontName + 4;
}

const uint8_t* oled_get_char_array(char Symbol) {
	const uint8_t* CharArray = Font;
	// Символ есть в шрифте.
	if (Symbol >= FontFirstSymbol && Symbol < FontFirstSymbol + FontSymbolsCount) {
		for (uint8_t i = 0; i < Symbol - FontFirstSymbol; i++) {
			CharArray += ((pgm_read_byte(CharArray) + 7) >> 3) * FontHeight + 1;
		}
	}
	else {return 0;}
	return CharArray + 1;
}

uint8_t oled_print_char(uint8_t x, uint8_t y, char Symbol) {
	const uint8_t* CharArray = Font;
	// Символ есть в шрифте.
	if (Symbol >= FontFirstSymbol && Symbol < FontFirstSymbol + FontSymbolsCount) {
		for (uint8_t i = 0; i < Symbol - FontFirstSymbol; i++) {
			CharArray += ((pgm_read_byte(CharArray) + 7) >> 3) * FontHeight + 1;
		}
	}
	else {return 0;}
	oled_draw_xbmp(x, y, CharArray + 1, pgm_read_byte(CharArray), FontHeight);
	return pgm_read_byte(CharArray);	// Возвращаем ширину напечатанного символа.
}

void oled_print_string(uint8_t x, uint8_t y, char* String, uint8_t MsgSize) {
	uint8_t DeltaX = 0;
	for (uint8_t i = 0; i < MsgSize; i++) {
		uint8_t CharW = oled_print_char(x + DeltaX, y, String[i]);
		if (CharW) {DeltaX += CharW + FontSpace;}
	}
}
/*
		0		1
	-----------------
	01234567 01234567

	0 1
	---
	0 0
	1 1
	2 2
	3 3
	4 4
	5 5
	6 6
	7 7

*/


/*
	0x80 — означает, что следом идёт один байт команды,
	0xC0 — означает, что следом идёт один байт данных,
	0x40 — следом идёт много байт данных.

	https://microsin.net/adminstuff/hardware/ssd1306-oled-controller.html
*/