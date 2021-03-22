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
#include "wdt.h"
#include "debug.h"

static bool wdt_started = false;

//void wdt_check(void) {
//
//	bool state = false;
//
//	int32_t count = 10;
//	/* Verifica en el arranque, si se ha producido timeout del WDT. En tal caso, parpadeará el led "spare" cada 1 seg */
//	if (Chip_WWDT_GetStatus(LPC_WWDT) & WWDT_WDMOD_WDTOF) {
//		while (--count) {
//			state = !state;
//			relay_spare_led(state);
//			udelay(1000000);
//		}
//	}
//
//	Chip_WWDT_ClearStatusFlag(LPC_WWDT,
//	WWDT_WDMOD_WDTOF | WWDT_WDMOD_WDINT);
//
//}

void wdt_init() {

	/* WDT Resets on 10 mseg without ticks from RTOS -vApplicationTicksHook- */
	int tc_mseg = 10;
	/* WDT_CLK Freq - divide by four (12Mhz/4) */

	uint32_t WDT_TO = ((WDT_OSC) / (4 * 1000)) * tc_mseg;

	uint32_t WDT_WINT = WDT_TO / 10;

	uint32_t WDT_WIND = WDT_TO;

	/* Initialize WWDT */
	Chip_WWDT_Init(LPC_WWDT);

	Chip_WWDT_SetTimeOut(LPC_WWDT, WDT_TO);

	/* -- Reset on WDT_TimeOutFLag -- */
	Chip_WWDT_SetOption(LPC_WWDT, WWDT_WDMOD_WDRESET);

	/* WINDOW: Max count value from wich a FEED can be credited */
	Chip_WWDT_SetWindow(LPC_WWDT, WDT_WIND);

	/* WARNING INTERRUPT */
	Chip_WWDT_SetWarning(LPC_WWDT, WDT_WINT);

	/* Clear TimeOut and Warning Flags, Time Out Flag (0x04); Warning flag (0x08) */
	Chip_WWDT_ClearStatusFlag(LPC_WWDT,
	WWDT_WDMOD_WDTOF | WWDT_WDMOD_WDINT);

	/* Start watchdog */
	Chip_WWDT_Start(LPC_WWDT); /* Enable and Feed */

	NVIC_ClearPendingIRQ(WWDT_IRQn);

	NVIC_EnableIRQ(WWDT_IRQn); /* Enable watchdog interrupt */

	Chip_WWDT_Feed(LPC_WWDT);
	Chip_WWDT_ClearStatusFlag(LPC_WWDT,
	WWDT_WDMOD_WDTOF | WWDT_WDMOD_WDINT);

	wdt_started = true;

}

void wdt_feed(){

	if(wdt_started){
		Chip_WWDT_Feed(LPC_WWDT);
	}
}

//void wdt_stop(){
//
//	LPC_WWDT->MOD = 0;
//
//}


void vApplicationTickHook(void) {


	if(WDT_ENABLED){

		wdt_feed();
	}

	if (WDT_TEST) {

		wdt_test();
	}
//
//	if (Chip_GPIO_GetPinState(LPC_GPIO_PORT, 3, 14)) {
//
//		HERE;
//
//	}
//
//	if (WDT_ENABLED) {
//
//		static bool first_cycle = TRUE;
//
//		if (first_cycle) {
//
//		}
//
//	}
}

