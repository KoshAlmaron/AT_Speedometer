// Управление шаговым двигателем.

#ifndef _SMCONTROL_H_
	#define _SMCONTROL_H_

	void sm_init();
	void sm_set_target(uint16_t Speed);
	void sm_step();

#endif