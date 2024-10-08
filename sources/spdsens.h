// Датчики скорости валов.

#ifndef _SPDSENS_H_
	#define _SPDSENS_H_
	
	// Смещения для увеличения точности расчета скорости.
	#define SPEED_BIT_SHIFT 2

	void set_impulse_per_km(uint16_t Count);
	uint16_t get_car_speed();
	uint32_t get_car_distance();

	extern volatile uint32_t Distance;

#endif