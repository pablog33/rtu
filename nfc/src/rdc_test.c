#include "rdc_test.h"

#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"
#include "debug.h"
#include "relay.h"
#include "ad2s1210.h"

#define RDC_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )

QueueHandle_t rdc_queue = NULL;

static struct ad2s1210 pole_rdc;

/**
 * @brief 	handles the Lift movement.
 * @param 	par		: unused
 * @return	never
 * @note	Receives commands from rdc_queue
 */
static void rdc_test_task(void *par)
{
	while (true) {

		uint32_t pos = ad2s1210_read_position(&pole_rdc);

		lDebug(Info, "tarea RDC_TEST pos: %i", pos);


		uint32_t fault_reg = ad2s1210_get_fault_register(&pole_rdc);
		lDebug(Info, "tarea RDC_TEST FAULT_REG: %i", fault_reg);

		ad2s1210_print_fault_register(&pole_rdc);

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle rdc movements.
 * @return	nothing
 */
void rdc_test_init()
{

	pole_rdc.gpios.reset = &poncho_rdc_reset;
	pole_rdc.gpios.sample = &poncho_rdc_sample;
	pole_rdc.gpios.wr_fsync = &poncho_rdc_pole_wr_fsync;
	pole_rdc.resolution = 16;
	pole_rdc.fclkin = 8192000;
	pole_rdc.fexcit = 2000;
	ad2s1210_init(&pole_rdc);

	xTaskCreate(rdc_test_task, "RDC_test", configMINIMAL_STACK_SIZE*2, NULL,
	RDC_TASK_PRIORITY, NULL);
	lDebug(Info, "rdc: task created");
}
