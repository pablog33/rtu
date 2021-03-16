#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "debug.h"
#include "temperature.h"

#define TEMPERATURE_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )

#define LPC_ADC LPC_ADC0
#define TEMP_ADC_CH ADC_CH1

/**
 * @brief 	initializes ADC to read temperature sensor.
 * @return	nothing
 */
void temperature_init()
{
	ADC_CLOCK_SETUP_T adc_setup;

	Chip_ADC_Init(LPC_ADC, &adc_setup);
	Chip_ADC_SetSampleRate(LPC_ADC, &adc_setup, 1000);
	Chip_ADC_EnableChannel(LPC_ADC, TEMP_ADC_CH, ENABLE);
	Chip_ADC_SetBurstCmd(LPC_ADC, ENABLE);
}

/**
 * @brief	returns status of the temperature task.
 * @return 	the temperature value in °C
 */
uint16_t temperature_read(void)
{
	static uint16_t temperature = 0;
	static TickType_t last_read = 0;

	TickType_t now = xTaskGetTickCount();

	if ((now - last_read) > pdMS_TO_TICKS(1000)) {
		/* Waiting for A/D conversion complete */
		while (Chip_ADC_ReadStatus(LPC_ADC, TEMP_ADC_CH, ADC_DR_DONE_STAT)
				!= SET) {
		}
		/* Read ADC value */
		Chip_ADC_ReadValue(LPC_ADC, TEMP_ADC_CH, &temperature);
		/* Print ADC value */
		lDebug(Info, "LECTURA ADC: %i", temperature);
		last_read = now;
	}

	return temperature;
}
