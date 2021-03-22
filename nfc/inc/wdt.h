/*
 * wdt.h
 *
 *  Created on: 25 feb. 2021
 *      Author: gspc
 */

#ifndef NFC_INC_WDT_H_
#define NFC_INC_WDT_H_

#ifdef WDT
#define WDT_ENABLED 1  // code available at runtime
#ifdef WDT_TEST
#define WDT_TEST 1  // code available at runtime
#else
#define WDT_TEST 0  // all code optimized out
#endif	/* WDT_TEST */
#else
#define WDT_ENABLED 0  // all code optimized out
#define WDT_TEST 0
#endif	/* WDT_ENABLED */

//void wdt_check(void);

void wdt_test(void);

//void din_init(void);

void wdt_feed(void);

void wdt_init(void);

void wdt_stop(void);

static bool wdt_started;

#endif /* NFC_INC_WDT_H_ */
