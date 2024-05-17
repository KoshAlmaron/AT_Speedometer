#include <avr/interrupt.h>		// Прерывания.
#include <stdint.h>				// Коротние название int.
#include "spdsens.h"			// Свой заголовок.

#define IMPULSE_PER_KM 6000LU	// Количество импульсов на 1 километр КПП ВАЗ.
//#define IMPULSE_PER_KM 25960	// Количество импульсов на 1 километр АКПП A650E.

// Для датчиков используются кольцевые буферы усреднения.
// Значения в массивах хранятся в числах АЦП - 0-1023.

// Измеренный период сигнала в шагах таймера (4 мкс).
volatile uint16_t OutputShaftPeriod = UINT16_MAX;
// Предыдущий нужен для определения случайных срабатываний.
volatile uint16_t OutputShaftPeriodPrev = UINT16_MAX;


// Кольцевые буферы для замера оборотов, прохождение 1 зуба шагах таймера (4 мкс).
#define BUFFER_SIZE 16
#define BUFFER_BITE_SHIFT 4
uint16_t SpeedArray[BUFFER_SIZE] = {0};
// Теккущая позиция в буфере.
uint8_t SpeedPos = 0;
// Обороты валов.
uint16_t CarSpeed = 0;

volatile uint16_t ImpulseCount = 0;	// Кол-во импульсов для расчета пробега.
volatile uint32_t Distance = 0;		// Общий пробег в метрах.

// Расчет скорости авто.
void calculate_car_speed() {
	// Считается по количеству импульсов на 1 км.
	// Обороты = 1000000 / ([Шаг таймера, мкс] * [Кол-во шагов]) * 3600 / [Импульсов на 1 км].
	// Обороты = (1000000 / (4 * Period)) * (3600 / 6000).
	// Обороты = (25 * 6000) 
	// Обороты = 150000 / Period.
	// Обороты = (25 * [Импульсов на 1 км]) / Period
	// Забираем значение из переменной с прерываниями.

	#define SPEED_CALC_COEF (25LU * IMPULSE_PER_KM)

	uint16_t Period = 0;
	cli();
		Period = OutputShaftPeriodPrev;
	sei();

	// Рассчитываем обороты вала.
	uint16_t Speed = 0;
	if (Period < UINT16_MAX && Period > 750) {
		Speed = (uint32_t) (SPEED_CALC_COEF << SPEED_BIT_SHIFT) / Period;
	}

	// Записываем значение в кольцевой буфер
	SpeedArray[SpeedPos] = Speed;
	SpeedPos++;
	if (SpeedPos >= BUFFER_SIZE) {SpeedPos = 0;}
	
	// Находим среднее значение.
	uint32_t AVG = 0;
	for (uint8_t i = 0; i < BUFFER_SIZE; i++) {AVG += SpeedArray[i];}
	CarSpeed = AVG >> BUFFER_BITE_SHIFT;
}

uint16_t get_car_speed() {
	return CarSpeed;
}

// Возвращает пробег автомобиля с метрами.
uint32_t get_car_distance() {
	cli();
		uint32_t DistFull = (Distance + ((uint32_t) ImpulseCount * 1000) / IMPULSE_PER_KM);
	sei();
	return DistFull;
}

// Прерывание по захвату сигнала таймером 1.
ISR (TIMER1_CAPT_vect) {
	// Обнулить счётный регистр.
	TCNT1 = 0;
	// Результат измерения берётся из регистра захвата.
	OutputShaftPeriodPrev = OutputShaftPeriod;
	OutputShaftPeriod = ICR1;

	// Подсчет пробега, IMPULSE_PER_KM импульсов метр.
	ImpulseCount++;
	if (ImpulseCount >= IMPULSE_PER_KM) {
		ImpulseCount = 0;
		Distance += 1000;
	}
}

// Прерывание по переполнению таймера 1.
ISR (TIMER1_OVF_vect) {
	OutputShaftPeriod = UINT16_MAX;
	OutputShaftPeriodPrev = UINT16_MAX;
}