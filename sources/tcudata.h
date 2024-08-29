// Структура данных.

#ifndef _TCUDATA_H_
	#define _TCUDATA_H_

	void draw_at_mode();

	// Структура для хранения переменных.
	typedef struct TCU_t {
		uint16_t DrumRPM;			// Обороты корзины овердрайва.
		uint16_t OutputRPM;			// Обороты выходного вала.
		uint8_t	CarSpeed;			// Скорость автомобиля.
		uint16_t SpdTimerVal;		// Значение регистра сравнения для таймера спидометра.
		int16_t OilTemp;			// Температура масла.
		uint16_t TPS;				// ДПДЗ.
		uint8_t	SLT;				// ШИМ, линейное давление.
		uint8_t	SLN;				// ШИМ, давление в гидроаккумуляторах.
		uint8_t	SLU;				// ШИМ, давление блокировки гидротрансформатора.
		uint8_t	S1;					// Соленоид № 1.
		uint8_t	S2;					// Соленоид № 2.
		uint8_t	S3;					// Соленоид № 3.
		uint8_t	S4;					// Соленоид № 4.
		uint8_t Selector;			// Положение селектора.
		uint8_t ATMode;				// Режим АКПП.
		int8_t Gear;				// Текущая передача.
		int8_t GearChange;			// Флаг идет процес переключения.
		uint8_t Break;				// Состояние педали тормоза.
		uint8_t EngineWork;			// Флаг работы двигателя.
		uint8_t SlipDetected;		// Обнаружено проскальзывание фрикционов.
		uint8_t Glock;				// Блокировка гидротрансформатора.
	} TCU_t;
	
	extern volatile struct TCU_t TCU; 		// Делаем структуру внешней.
	extern volatile uint8_t DataStatus;

#endif