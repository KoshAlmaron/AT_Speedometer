#include <stdint.h>				// Коротние название int.
#include <avr/interrupt.h>		// Прерывания.
#include <avr/io.h>				// Названия регистров и номера бит.
#include <stdio.h>				// Стандартная библиотека ввода/вывода.
#include <avr/eeprom.h>			// Работа с EEPROM.
#include <avr/pgmspace.h>		// Работа с PROGMEM.
#include <avr/wdt.h>			// Сторожевой собак.

#include "macros.h"				// Макросы.
#include "pinout.h"				// Начначение выводов контроллера.
#include "timers.h"				// Таймеры.

#include "spdsens.h"			// Датчики скорости валов.
#include "smcontrol.h"			// Управление шаговым двигателем.

#include "i2c.h"				// I2C (TWI).
#include "oled.h"				// Управление OLED на базе SSD1306.

#include "fonts.h"				// Шрифты.
#include "tcudata.h"			// Данные от АКПП.
#include "uart.h"				// UART.
#include "configuration.h"		// Настройки.


//#define DEBUG_MODE
#ifdef DEBUG_MODE
	#include "uart.h"	// UART
	uint16_t SpeedTest = 0;
#endif

// Основной счетчик времени,
// увеличивается по прерыванию на единицу каждую 1мс.
volatile uint8_t MainTimer = 0;

// Счетчики времени.
uint16_t SensorTimer = 0;
uint16_t OledUpTimer = 0;
int16_t OledDownTimer = -300;
uint16_t SecondsTimer = 0;
int16_t AlarmBoxTimer = 0;

#define Digit_width 13
#define Digit_height 26

// Флаг включения отображения состояния АКПП на нижний экран.
uint8_t ATModeShow = 0;

// Прототипы локальных функций.
static void loop();
static void eeprom_read();
static void eeprom_write();
static void oled_odometer(uint32_t DistanceKM);
static void oled_speed(uint16_t Speed);
static void oled_at_mode();
static void draw_animation(uint8_t x, char CurrDigit, char NextDigit, uint8_t Offset);
static void enable_int0();
static void selector_pins_init();
static void display_select(uint8_t N);

int main() {
	wdt_enable(WDTO_500MS);	// Сторожевой собак на 500 мс.
	cli();	
		sm_init();			// Настройка ШД.
		timers_init();		// Настройка таймеров.
		i2c_init();			// Настройка I2C.
		enable_int0();		// Прерывание при выключении ЗЗ.
		eeprom_read();
		uart_init(1);
	sei();

	// Вывод со светодиодом.
	SET_PIN_MODE_OUTPUT(LED_PIN);
	SET_PIN_LOW(LED_PIN);

	#ifdef DEBUG_MODE
		/*============== Отладка ==============*/
		uart_init(2);
		#define SPD_STEP_UP_PIN D, 7
		#define SPD_STEP_DOWN_PIN D, 6
		SET_PIN_MODE_INPUT(SPD_STEP_UP_PIN);
		SET_PIN_HIGH(SPD_STEP_UP_PIN);
		SET_PIN_MODE_INPUT(SPD_STEP_DOWN_PIN);
		SET_PIN_HIGH(SPD_STEP_DOWN_PIN);
		/*============== Отладка ==============*/
	#endif

	selector_pins_init();	// Настройка портов переключения дисплея.
	display_select(0);
	oled_init(0x3c, 60, 1);	// Настройка OLED 0.

	display_select(1);
	oled_init(0x3c, 100, 0);	// Настройка OLED 1.

	while(1) {
		loop();
	}
	return 0;
}

static void eeprom_read() {
	Distance = eeprom_read_dword(0);	// 0-3
	//eeprom_write();
}

static void eeprom_write() {
	//Distance = 182400000; // 14.05.2024
	eeprom_write_dword(0, get_car_distance());	// 0-3
}

// Основной цикл.
static void loop() {
	wdt_reset();		// Сброс сторожевого таймера.
	// Временно выключаем прерывания, чтобы забрать значение счетчика.
	uint8_t TimerAdd = 0;
	cli();
		TimerAdd = MainTimer;
		MainTimer = 0;
	sei();

	// Счетчики времени.
	if (TimerAdd) {
		SensorTimer += TimerAdd;
		OledUpTimer += TimerAdd;		
		OledDownTimer += TimerAdd;
		SecondsTimer += TimerAdd;

		AlarmBoxTimer += TimerAdd;
		if (AlarmBoxTimer > 800) {AlarmBoxTimer = -800;}
	}

	// Обработка датчиков.
	if (SensorTimer >= 50) {
		SensorTimer = 0;
		calculate_car_speed();
	}

	if (SecondsTimer >= 1000) {
		SecondsTimer = 0;

		#ifdef DEBUG_MODE
			Distance += 500;
			SpeedTest += 5 << SPEED_BIT_SHIFT;
			if (SpeedTest >= 185) {SpeedTest = 0;}
			//PIN_TOGGLE(LED_PIN);
		#endif
	}

	// Вывод пробега на верхний OLED.
	if (OledUpTimer >= 35 && i2c_ready()) {
		OledUpTimer = 0;
		oled_odometer(Distance / 1000);
	}

	// Если есть данные с АКПП, то переключаем режим нижнего экрана.
	if (!ATModeShow && DataStatus == 2) {
		ATModeShow = 1;
		set_impulse_per_km(IMPULSE_PER_KM_AT);
	}

	// Вывод скорости или состояния АКПП на нижний OLED.
	if (OledDownTimer >= 150 && i2c_ready()) {
		OledDownTimer = 0;

		#ifdef DEBUG_MODE
			oled_speed(SpeedTest);
			sm_set_target(SpeedTest);
		#else
			sm_set_target(get_car_speed());		// Установка стрелки.
			if (ATModeShow) {oled_at_mode();}	// Состояние АКПП.
			else {oled_speed(get_car_speed());}	// Скорость.
		#endif
	}
}

static void oled_odometer(uint32_t DistanceKM) {
	//return;
	static uint8_t Offset = 0;
	static uint32_t DistanceCurr = 0;
	static uint32_t DistancePrev = 0;

	// Буферы для отображения пробега в виде строки.
	char MileAgeCurr[7] = {0};
	char MileAgePrev[7] = {0};

	if (DistancePrev == DistanceKM && !Offset) {return;}
	if (!DistanceCurr) {DistanceCurr = DistanceKM;}
	if (!Offset) {DistanceCurr = DistanceKM;}

	snprintf(MileAgeCurr, 7, "%6lu", DistanceCurr);
	if (DistancePrev) {snprintf(MileAgePrev, 7, "%6lu", DistancePrev);}

	oled_clear_buffer();
	oled_set_font(Font_Logisoso_26_tn);
	for (uint8_t i = 0; i < 6; i++) {
		if (MileAgeCurr[i] == MileAgePrev[i]) {
			oled_print_char(i * 19 + 10, 3, MileAgeCurr[i]);
		}
		else {
			draw_animation(i * 19 + 10, MileAgePrev[i], MileAgeCurr[i], Offset);
		}
	}
	if (DistanceCurr != DistancePrev) {Offset++;}
	if (Offset > 32) {
		Offset = 0;
		DistancePrev = DistanceCurr;
	}

	display_select(0);
	oled_send_data();
}

static void oled_speed(uint16_t Speed) {
	Speed = Speed >> SPEED_BIT_SHIFT;

	// Буферы для отображения скорости в виде строки.
	char SpeedStr[4] = {0};
	snprintf(SpeedStr, 4, "%3u", Speed);
	oled_clear_buffer();

	oled_set_font(Font_Logisoso_32_tn);

	uint8_t Shift = 58;
	if (Speed > 9) {Shift = 48;}
	if (Speed > 99) {Shift = 37;}

	oled_print_string(Shift, 0, SpeedStr, 3);
	display_select(1);
	oled_send_data();
}

static void oled_at_mode() {
	if (DataStatus != 2) {return;}
	oled_clear_buffer();

	// oled_set_font(At_modes_22x11);
	// for (uint8_t i = 0; i < 7; i++) {
	// 	oled_print_char(17*i , 0, i+4);
	// }
	// oled_send_data();
	// return;

	oled_set_font(At_modes_22x11);
	oled_print_char(6, 5, TCU.Selector);
	oled_print_char(27, 5, TCU.ATMode);
	oled_draw_frame(23, 1, 19, 30);

	oled_set_font(Gears_32x16);
	oled_print_char(60, 0, TCU.Gear + 1);

	oled_draw_v_line(52, 0, 32);
	oled_draw_v_line(53, 0, 32);
	oled_draw_v_line(82, 0, 32);
	oled_draw_v_line(83, 0, 32);

	oled_set_font(Font_Logisoso_22_tn);
	char Str[4] = {0};
	snprintf(Str, 4, "%3i", TCU.OilTemp);
	for (uint8_t i = 0; i < 4; i++) {
		if (Str[i] == ' ') {Str[i] = 0x3b;}		// Вставка пустышки.
		if (Str[i] == '-') {Str[i] = 0x3a;}		// Вставка знака минусы.
	}
	oled_print_string(88, 5, Str, 3);

	if (TCU.Glock) {
		oled_draw_v_line(49, 0, 32);
		oled_draw_v_line(50, 0, 32);
		oled_draw_v_line(85, 0, 32);
		oled_draw_v_line(86, 0, 32);
	}

	if (AlarmBoxTimer > 0) {
		oled_draw_mode(1);
		if (TCU.OilTemp > OIL_MAX_TEMP) {	// Индикация перегрева.
			if (TCU.OilTemp < 100) {oled_draw_box(98, 3, 30, 26);}
			else {oled_draw_box(88, 3, 40, 26);}
		}
		if (TCU.Selector == 9) {			// Ошибка селектора.
			oled_draw_box(4, 2, 15, 28);
		}
		if (TCU.ATMode == 9) {				// Ошибка АКПП.
			oled_draw_box(24, 2, 17, 28);
		}
		if (TCU.SlipDetected) {				// Обнаружено проскальзывание фрикционов.
			oled_draw_box(57, 0, 22, 32);
		}
		oled_draw_mode(0);
	}

	display_select(1);
	oled_send_data();
	DataStatus = 0;
}

static void draw_animation(uint8_t x, char CurrDigit, char NextDigit, uint8_t Offset) {
	const uint8_t* CurrXBM = oled_get_char_array(CurrDigit);
	const uint8_t* NextXBM = oled_get_char_array(NextDigit);
	CurrXBM+= ((Digit_width + 7) >> 3) * Offset;

	uint8_t CurrY = max(0, 3 - Offset);
	if (Offset <= Digit_height && CurrDigit) {
		oled_draw_xbmp(x, CurrY, CurrXBM, Digit_width, Digit_height - Offset);
	}
	if (NextDigit) {
		oled_draw_xbmp(x, 3 + 32 - Offset, NextXBM, Digit_width, Digit_height);
	}
}

static void selector_pins_init() {
	SET_PIN_MODE_OUTPUT(OLED_0_EN_PIN);
	SET_PIN_LOW(OLED_0_EN_PIN);

	SET_PIN_MODE_OUTPUT(OLED_1_EN_PIN);
	SET_PIN_LOW(OLED_1_EN_PIN);
}

static void display_select(uint8_t N) {
	SET_PIN_LOW(OLED_0_EN_PIN);
	SET_PIN_LOW(OLED_1_EN_PIN);

	if (N) {SET_PIN_HIGH(OLED_1_EN_PIN);}
	else {SET_PIN_HIGH(OLED_0_EN_PIN);}
}

// Прерывание INT0 на спадающий фронт сигнала.
static void enable_int0() {
	// Пин как вход без подтяжки.
	DDRD &= ~(1 << 2);
	PORTD &= ~(1 << 2);

	EICRA |= (1 << ISC01);		// Falling edge.
	EIMSK |= (1 << INT0);		// INT0 On
}

// Прерывание при выключении ЗЗ.
ISR (INT0_vect) {
	eeprom_write();
	PIN_TOGGLE(LED_PIN);
}

// Прерывание при совпадении регистра сравнения OCR0A на таймере 0 каждую 1мс. 
ISR (TIMER0_COMPA_vect) {
	MainTimer++;
}