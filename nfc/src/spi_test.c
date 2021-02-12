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

static struct ad2s1210 rdc;

static void spi_test_task(void *par)
{
	while (true) {
//		ad2s1210_hard_reset(&rdc);
//		ad2s1210_soft_reset(&rdc);

		//ad2s1210_read_position(&rdc);
//		ad2s1210_get_reg(&rdc, AD2S1210_REG_EXCIT_FREQ);
		ad2s1210_print_fault_register(&rdc);
		ad2s1210_clear_fault_register(&rdc);
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void spi_test_init()
{
	rdc.gpios.reset = &poncho_rdc_reset;
	rdc.gpios.sample = &poncho_rdc_sample;
	rdc.gpios.wr_fsync = &poncho_rdc_pole_wr_fsync;
	rdc.resolution = 16;
	rdc.fclkin = 8192000;
	rdc.fexcit = 2000;
	ad2s1210_init(&rdc);

	xTaskCreate(spi_test_task, "SPI_Test", 512, NULL,
	6, NULL);

	lDebug(Info, "SPI_TEST task created");
}
