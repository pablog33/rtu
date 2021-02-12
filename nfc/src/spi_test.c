#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"

#include "ad2s1210.h"
#include "debug.h"
#include "dout.h"
#include "pid.h"
#include "tmr.h"

static struct ad2s1210 rdc_pole;
static struct ad2s1210 rdc_arm;

static void spi_test_task(void *par)
{
	while (true) {
		ad2s1210_print_fault_register(&rdc_arm);
		ad2s1210_clear_fault_register(&rdc_arm);
		int posAct = ad2s1210_read_position(&rdc_arm);

		lDebug(Info, "***********************");
		lDebug(Info, "Pos: %d", posAct);
		lDebug(Info, "***********************");

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void spi_test_init()
{
	rdc_pole.gpios.reset = &poncho_rdc_reset;
	rdc_pole.gpios.sample = &poncho_rdc_sample;
	rdc_pole.gpios.wr_fsync = &poncho_rdc_pole_wr_fsync;
	rdc_pole.resolution = 16;
	rdc_pole.fclkin = 8192000;
	rdc_pole.fexcit = 2000;
	ad2s1210_hard_reset(&rdc_pole);
	ad2s1210_init(&rdc_pole);

	rdc_arm.gpios.reset = &poncho_rdc_reset;
	rdc_arm.gpios.sample = &poncho_rdc_sample;
	rdc_arm.gpios.wr_fsync = &poncho_rdc_arm_wr_fsync;
	rdc_arm.resolution = 16;
	rdc_arm.fclkin = 8192000;
	rdc_arm.fexcit = 2000;
	ad2s1210_hard_reset(&rdc_arm);
	ad2s1210_init(&rdc_arm);

	xTaskCreate(spi_test_task, "SPI_Test", 512, NULL,
	6, NULL);

	lDebug(Info, "SPI_TEST task created");
}

