#include <stdint.h>				// Коротние название int.

#include "tcudata.h"			// Свой заголовок.

// Статусы данных от АКПП:
// 0 - Готов к приему,
// 1 - идет прием пакета,
// 2 - пакет принят, ожидание обработки,
// 3 - ошибка приема.
volatile uint8_t DataStatus = 0;

// Инициализация структуры
volatile TCU_t TCU = {
	.DrumRPM = 0,
	.OutputRPM = 0,
	.CarSpeed = 0,
	.OilTemp = 0,
	.TPS = 0,
	.InstTPS = 0,
	.SLT = 0,
	.SLN = 0,
	.SLU = 0,
	.S1 = 0,
	.S2 = 0,
	.S3 = 0,
	.S4 = 0,
	.Selector = 0,
	.ATMode = 0,
	.Gear = 0,
	.GearChange = 0,
	.GearStep = 0,
	.LastStep = 0,
	.Gear2State = 0,
	.Break = 0,
	.EngineWork = 0,
	.SlipDetected = 0,
	.Glock = 0,
	.GearUpSpeed = 0,
	.GearDownSpeed = 0,
	.GearChangeTPS = 0,
	.GearChangeSLT = 0,
	.GearChangeSLN = 0,
	.GearChangeSLU = 0,
	.LastPDRTime = 0,
	.CycleTime = 0,
	.DebugMode = 0
};