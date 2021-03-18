/*
 * wdt.c
 *
 *  Created on: 25 feb. 2021
 *      Author: gspc
 */

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "dout.h"
#include "relay.h"

void wdt_check(void)
{

	bool state = false;

	int32_t count = 10;
	/* Verifica en el arranque, si se ha producido timeout del WDT. En tal caso, parpadeará el led "spare" cada 1 seg */
	if (Chip_WWDT_GetStatus(LPC_WWDT) & WWDT_WDMOD_WDTOF) {
		while (--count) {
			state = !state;
			relay_spare_led(state);
			udelay(1000000);
		}
	}

	Chip_WWDT_ClearStatusFlag(LPC_WWDT,
	WWDT_WDMOD_WDTOF | WWDT_WDMOD_WDINT);

}

void vApplicationTickHook(void)
{
//	static uint32_t test = 10000;
//
//	static uint16_t led_wdt = 1000;
//
//	/* -----------------------------------------------------*/
//	/* -- spare led flashing 1 hz, when entering appTickHook*/
//	if (!test) {
//		configASSERT(0);
//		test = 10000;
//	}
//	--test;
//	/* ---------------------------------------------------------*/
//
//	/* -----------------------------------------------------*/
//	/* -- spare led flashing 1 hz, when entering appTickHook*/
//	if (!led_wdt) {
//		dout_arm_pulse();
//		led_wdt = 1000;
//	}
//	--led_wdt;
//	/* ---------------------------------------------------------*/

	static bool first_cycle = TRUE;

	if (first_cycle) {

		int tc_mseg = 10;

		uint32_t WDT_TO = ((WDT_OSC) / (4 * 1000)) * tc_mseg;

		uint32_t WDT_WINT = WDT_TO / 10;

		uint32_t WDT_WIND = WDT_TO;

		/* Initialize WWDT */
		Chip_WWDT_Init(LPC_WWDT);

		Chip_WWDT_SetTimeOut(LPC_WWDT, WDT_TO);

		Chip_WWDT_SetWarning(LPC_WWDT, WDT_WINT); /* WINDOW: Max count value from wich a FEED can be credited */

		Chip_WWDT_SetWindow(LPC_WWDT, WDT_WIND);  /* WINDOW: Max count value from wich a FEED can be credited */

		/* Enable Reset on Time out */
		Chip_WWDT_SetOption(LPC_WWDT, WWDT_WDMOD_WDRESET);

		Chip_WWDT_ClearStatusFlag(LPC_WWDT,
		WWDT_WDMOD_WDTOF | WWDT_WDMOD_WDINT); /* Time Out Flag (0x04); Warning flag (0x08) */

		/* Start watchdog */
		Chip_WWDT_Start(LPC_WWDT); /* Enable and Feed */

		NVIC_ClearPendingIRQ(WWDT_IRQn);

		NVIC_EnableIRQ(WWDT_IRQn); /* Enable watchdog interrupt */

		Chip_WWDT_Feed(LPC_WWDT);
		Chip_WWDT_ClearStatusFlag(LPC_WWDT,
		WWDT_WDMOD_WDTOF | WWDT_WDMOD_WDINT);

		first_cycle = FALSE;
	}
	else {
		Chip_WWDT_Feed(LPC_WWDT);
		Chip_WWDT_ClearStatusFlag(LPC_WWDT,
		WWDT_WDMOD_WDTOF | WWDT_WDMOD_WDINT);
	}
}

//void WDT_IRQHandler(void)
//{
//	while (1);
//}
