#include <stdint.h>				// Коротние название int.
#include <avr/io.h>				// Названия регистров и номера бит.
#include <avr/interrupt.h>		// Прерывания.

#include "macros.h"				// Макросы.
#include "pinout.h"				// Начначение выводов контроллера.
#include "smcontrol.h"			// Свой заголовок.
#include "uart.h"				// UART.
#include "spdsens.h"			// Датчики скорости валов.
/*
	Управление шаговым двигателем происходит полностью из прерываний,
	так как важна плавность хода стрелки, а частое обновление экранов
	не требуется.
*/

#define SM_MAX_STEPS 740		// Максимальное количество шагов ШД.
#define SM_INIT_STEPS 620		// Количество шагов ШД до конца циферблата.
#define ACC_MAP_MAX 10			// Максимальная позиция в карте.

volatile uint8_t SMInit = 0;	// Флаг инициализации.

uint8_t AccMap[] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4};	// Карта ускорения.
volatile uint8_t ACCPos = ACC_MAP_MAX;					// Текущая позиция ускорения.
volatile uint16_t SPDTimer = 0;							// Таймер для управления скоростью ШД.

volatile uint16_t Position = 0;						// Текущая позиция.
volatile uint16_t Target = 0;						// Целевая позиция.

// Состояние ШД, 0 - простой,
// 1 - движение по часовой,
// 2 - против часовой.
volatile int8_t Direction = 0;

// Таблиц соответствия шагов и сокрости.
uint16_t StepArray[21] = {0, 39, 74, 105, 139, 174, 209, 243, 278, 313, 346, 380, 414, 449, 482, 516, 549, 584, 618, 652, 686};

static uint16_t get_interpolated_value(uint16_t x, uint16_t* ArrayY, uint8_t ArraySize);

// Настройка выводов.
void sm_init() {
	SET_PIN_MODE_OUTPUT(SM_DIR_PIN);
	SET_PIN_MODE_OUTPUT(SM_STEP_PIN);
	SET_PIN_LOW(SM_DIR_PIN);
	SET_PIN_LOW(SM_STEP_PIN);

	// Движение стрелки в максимум.
	Target = SM_INIT_STEPS;
}

void sm_set_target(uint16_t Speed) {
	if (!SMInit) {return;}

	int8_t Dir = 0;
	cli();
		Dir = Direction;
	sei();

	Speed += 1 << SPEED_BIT_SHIFT;		// Добавка к показанию.

	if (!Dir) {Target = get_interpolated_value(Speed, StepArray, 21);}
	sei();
}

// Управление ШД, функция вызывается каждую 1 мс.
void sm_step() {
	SPDTimer++;
	if (Direction) {
		if (SPDTimer > AccMap[ACCPos]) {
			SPDTimer = 0;

			PIN_TOGGLE(SM_STEP_PIN);
			if (!PIN_READ(SM_STEP_PIN)) {
				Position += Direction;

				uint16_t StepsLeft = 0;
				if (Target > Position) {StepsLeft = Target - Position;}
				else {StepsLeft = Position - Target;}

				// Замедление.
				if (StepsLeft <= ACC_MAP_MAX) {
					if (ACCPos < ACC_MAP_MAX) {ACCPos++;}
				}
				else {	// Ускорение.
					if (ACCPos) {ACCPos--;}
				}

				if (Position == Target) {
					Direction = 0;

					// Возвращение стрелки в 0 после инициализации.
					if (!SMInit && Position == SM_INIT_STEPS) {
						SMInit = 1;
						Target = 0;
						Position = SM_MAX_STEPS;
					}
				}
			}
		}
	}
	else {
		if (Position < Target) {		// Двигаемся по часовой.
			Direction = 1;
			SET_PIN_HIGH(SM_DIR_PIN);
		}
		else if (Position > Target) {	// Двигаемся против часовой.
			Direction = -1;
			SET_PIN_LOW(SM_DIR_PIN);			
		}
		ACCPos = ACC_MAP_MAX;
	}
}

// Возвращаент интерполированное значение из графика
static uint16_t get_interpolated_value(uint16_t x, uint16_t* ArrayY, uint8_t ArraySize) {
	uint16_t Result = 0;
	uint8_t StepX = 10 << SPEED_BIT_SHIFT; 

	if (x <= 0) {return 0;}
	if (x >= (ArraySize - 1) * StepX) {return ArrayY[ArraySize - 1];}		

	// Находим позицию в графике.
	for (uint8_t i = 0; i < ArraySize; i++) {
		if (x <= i * StepX) {
			// Находим значение с помощью интерполяции.
			uint16_t x0 = (i - 1) * StepX;
			uint16_t x1 = i * StepX;
			
			uint16_t y0 = ArrayY[i - 1];
			uint16_t y1 = ArrayY[i];
			Result = y0 * 16 + (((y1 - y0) * 16) * (x - x0)) / (x1 - x0);
			break;
		}
	}

	Result = Result / 16;
	return Result;
}

// Прерывание при совпадении регистра сравнения OCR2A на таймере 2. 
ISR (TIMER2_COMPA_vect){
	sm_step();
}