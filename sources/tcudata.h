// Структура данных.

#ifndef _TCUDATA_H_
	#define _TCUDATA_H_

	void draw_at_mode();

	// Структура для хранения переменных.
	typedef struct TCU_t {
		uint16_t DrumRPM;			// Обороты корзины овердрайва.
		uint16_t OutputRPM;			// Обороты выходного вала.
		uint8_t CarSpeed;			// Скорость автомобиля.
		int16_t OilTemp;			// Температура масла.
		uint16_t TPS;				// ДПДЗ усредненый для расчетов.
		uint16_t InstTPS;			// ДПДЗ мгновенный.
		uint16_t SLT;				// ШИМ, линейное давление.
		uint16_t SLN;				// ШИМ, давление в гидроаккумуляторах.
		uint16_t SLU;				// ШИМ, давление блокировки гидротрансформатора.
		uint8_t S1;					// Соленоид № 1.
		uint8_t S2;					// Соленоид № 2.
		uint8_t S3;					// Соленоид № 3.
		uint8_t S4;					// Соленоид № 4.
		uint8_t Selector;			// Положение селектора.
		uint8_t ATMode;				// Режим АКПП.
		int8_t Gear;				// Текущая передача.
		int8_t GearChange;			// Флаг идет процес переключения.
		uint8_t GearStep;			// Номер шага процесса переключения.
		uint8_t LastStep;			// Последний шаг при переключении 1>2 или 2>3.
		uint8_t Gear2State;			// Состояние второй передачи.
		uint8_t Break;				// Состояние педали тормоза.
		uint8_t EngineWork;			// Флаг работы двигателя.
		uint8_t SlipDetected;		// Обнаружено проскальзывание фрикционов.
		uint8_t Glock;				// Блокировка гидротрансформатора.
		uint8_t GearUpSpeed;		// Скорость переключения вверх.
		uint8_t GearDownSpeed;		// Скорость переключения вниз.
		uint8_t GearChangeTPS;		// ДПДЗ в начале переключения.
		uint16_t GearChangeSLT;		// SLT в начале переключения.
		uint16_t GearChangeSLN;		// SLN в начале переключения.
		uint16_t GearChangeSLU;		// SLU в начале переключения.
		uint16_t LastPDRTime;		// Последнее время работы PDR.
		uint16_t CycleTime;			// Время цикла.
		uint8_t DebugMode;			// Режим отладки.
	} TCU_t;
	
	extern volatile struct TCU_t TCU; 		// Делаем структуру внешней.
	extern volatile uint8_t DataStatus;

#endif