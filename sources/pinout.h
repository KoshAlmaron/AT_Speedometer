	// Начначение выводов контроллера.

#ifndef _PINOUT_H_
	#define _PINOUT_H_

	#define SM_DIR_PIN D, 4
	#define SM_STEP_PIN D, 5
	
	#define OLED_0_EN_PIN C, 3
	#define OLED_1_EN_PIN C, 2

	#define LED_PIN B, 5

#endif

/*
	0 PD0	(RX)			|
	1 PD1	(TX)			|
	2 PD2	(INT0)			|	Прерывание по выключению ЗЗ.
   ~3 PD3	(INT1) (OC2B)	|
	4 PD4					|	ШД направление.
   ~5 PD5	(OC0B)			|	ШД шаг.
   ~6 PD6	(OC0A)			|
	7 PD7					|
	8 PB0	(ICP1)			|	Датчик скорости.
   ~9 PB1	(OC1A)			|
   ~10 PB2	(OC1B) (SS)		|
   ~11 PB3	(OC2A) (MOSI)	|
	12 PB4	(MISO)			|
	13 PB5	(SCL) (LED)		|	Светодиод для контроля работы.

	A0 PC0					| 
	A1 PC1					|
	A2 PC2					|	Включение экрана 1.
	A3 PC3					|	Включение экрана 0.
	A4 PC4	(SDA)			|	I2C
	A5 PC5	(SCL)			|	I2C
*/