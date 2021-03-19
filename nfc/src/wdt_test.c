/*
 * wdt.c
 *
 *  Created on: 25 feb. 2021
 *      Author: gspc
 */

#include <stdbool.h>

#include "board.h"
#include "dout.h"
#include "relay.h"
#include "wdt.h"

void wdt_test(void) {

	/* --	HookTicks call freq: 1000 Hz, hence count expires
	 * -WDT Timeout- each  -wdt_testHookTickCount * 1 mseg- --	*/
	static uint32_t wdt_testHookTickCount = 10000;

	/* --	HookTicks call freq: 1000 Hz, hence count expires
	 * -spareLedFlashes- each  -wdt_testled * 1 mseg- --	*/
	static uint16_t wdt_testLed = 1000;

	/* -----------------------------------------------------*/
	/* -- Force WDT Reset on -- */
	if (!wdt_testHookTickCount) {
		configASSERT(0);
		wdt_testHookTickCount = 10000;
	}
	--wdt_testHookTickCount;
	/* ---------------------------------------------------------*/

	/* -----------------------------------------------------*/
	/* -- spare led flashing 1 hz, when entering appTickHook*/
	if (!wdt_testLed) {
		dout_arm_pulse();
		wdt_testLed = 1000;
	}
	--wdt_testLed;
	/* ---------------------------------------------------------*/

}

//void WDT_IRQHandler(void)
//{
//	while (1);
//}
