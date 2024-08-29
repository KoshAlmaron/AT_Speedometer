#include <avr/interrupt.h>		// Прерывания.
#include <stdint.h>				// Коротние название int.

#include "configuration.h"		// Настройки.
#include "spdsens.h"			// Свой заголовок.

// Минимальное сырое значения для фильтрации ошибочных значений.
#define MIN_RAW_VALUE 830 		// ~180км/ч.

// Кольцевые буферы для замера оборотов, прохождение 1 зуба шагах таймера (4 мкс).
#define BUFFER_SIZE 16 + 4
#define BUFFER_BITE_SHIFT 4

uint16_t CarSpeed = 0;

volatile uint16_t ImpulseArray[BUFFER_SIZE] = {0};		// Кольцевой буфер.
volatile uint8_t BufPos = 0;							// Текущая позиция.
volatile uint8_t BufferReady = 1;		// Признак готовности буфера для записи.
volatile uint16_t ImpulseCount = 0;		// Кол-во импульсов для расчета пробега.
volatile uint32_t Distance = 0;			// Общий пробег в метрах.

// Количество импульсов на 1 километр
uint16_t ImpulsePerKM = IMPULSE_PER_KM_MT;

// Расчет скорости авто.
uint16_t get_car_speed() {
	// Считается по количеству импульсов на 1 км.
	// Скорость = 1000000 / ([Шаг таймера, мкс] * [Кол-во шагов]) * 3600 / [Импульсов на 1 км].
	// Скорость = (1000000 / (4 * Period)) * (3600 / 6000).
	// Скорость = 150000 / Period.
	// Скорость = (25 * [Импульсов на 1 км]) / Period

	#define SPEED_CALC_COEF (uint32_t) (25LU * ImpulsePerKM)
	
	BufferReady = 0;	// Запрещаем обновление буфера в прерываниях.

	// Переменные для хранения двух крайних значений.
	uint16_t MaxValue = ImpulseArray[0];
	uint16_t MaxValuePrev = ImpulseArray[0];
	
	uint16_t MinValue = ImpulseArray[0];
	uint16_t MinValuePrev = ImpulseArray[0];
	
	uint32_t AVG = 0;
	// Суммируем среднее и ищем крайние значения.
	for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
		AVG += ImpulseArray[i];

		if (ImpulseArray[i] < MinValue) {
			MinValuePrev = MinValue;
			MinValue = ImpulseArray[i];
		}
		if (ImpulseArray[i] > MaxValue) {
			MaxValuePrev = MaxValue;
			MaxValue = ImpulseArray[i];
		}
	}
	BufferReady = 1;	// Разрешаем обновление буфера в прерываниях.

	// Исключаем два самых больших и два самых маленьких значения.
	AVG -= (MaxValue + MaxValuePrev + MinValue + MinValuePrev);
	// Находим среднее значение.
	AVG = AVG >> BUFFER_BITE_SHIFT;

	// Рассчитываем скорость.
	uint16_t Speed = 0;
	if (AVG < UINT16_MAX - 100) {
		Speed = (uint32_t) (SPEED_CALC_COEF << SPEED_BIT_SHIFT) / AVG;
	}
	return Speed;
}

void set_impulse_per_km(uint16_t Value) {
	ImpulsePerKM = Value;
}

// Возвращает пробег автомобиля с метрами.
uint32_t get_car_distance() {
	cli();
		uint32_t DistFull = (Distance + ((uint32_t) ImpulseCount * 1000) / ImpulsePerKM);
	sei();
	return DistFull;
}

// Прерывание по захвату сигнала таймером 1.
ISR (TIMER1_CAPT_vect) {
	// Обнулить счётный регистр.
	TCNT1 = 0;

	// Фильтр на значения выше 180 км/ч.
	if (BufferReady && ICR1 > MIN_RAW_VALUE) {
		// Записываем значение в кольцевой буфер.	
		ImpulseArray[BufPos] = ICR1;
		BufPos++;
		if (BufPos >= BUFFER_SIZE) {BufPos = 0;}
	}

	// Подсчет пробега, ImpulsePerKM импульсов метр.
	ImpulseCount++;
	if (ImpulseCount >= ImpulsePerKM) {
		ImpulseCount = 0;
		Distance += 1000;
	}
}

// Прерывание по переполнению таймера 1.
ISR (TIMER1_OVF_vect) {
	for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
		ImpulseArray[i] = UINT16_MAX;
	}
}