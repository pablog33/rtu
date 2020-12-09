

#include "board.h"
#include "arch/lpc18xx_43xx_emac.h"
#include "arch/lpc_arch.h"
#include "arch/sys_arch.h"
#include "lpc_phy.h" /* For the PHY monitor support */
#include <rtu_com_hmi.h>
#include "dout.h"
#include "debug.h"



/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

uint8_t __attribute__((section ("." "data" ".$" "RamLoc40"))) ucHeap[ configTOTAL_HEAP_SIZE ]; /* GPa 201117 1850 Iss2: agregado de Heap_4.c*/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();
	dout_init();

	/* Initial LED DOUT4 state is off to show an unconnected cable state */
	Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 5, 12); /* LOW */
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	MilliSecond delay function based on FreeRTOS
 * @param	ms	: Number of milliSeconds to delay
 * @return	Nothing
 * Needed for some functions, do not use prior to FreeRTOS running
 */
void msDelay(uint32_t ms)
{
	vTaskDelay((configTICK_RATE_HZ * ms) / 1000);
}

/**
 * @brief	main routine for example_lwip_tcpec+ho_freertos_18xx43xx
 * @return	Function should not exit
 */
int main(void)
{
	prvSetupHardware();
	debugSetLevel(Info);

	/* Add another thread for initializing physical interface. This
	   is delayed from the main LWIP initialization. */
	xTaskCreate(vStackIpSetup, "StackIpSetup",
				configMINIMAL_STACK_SIZE*4, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

/*-----------------------------------------------------------*/
/* GPa 201110 1400 */
/**
 * @brief	configASSERT callback function
 * @param 	ulLine		: line where configASSERT was called
 * @param 	pcFileName	: file where configASSERT was called
 */

/* This function must be defined in a C source file, not the FreeRTOSConfig.h header file. */
//void vAssertCalled( const char *pcFile, uint32_t ulLine )
//{
///* Inside this function, pcFile holds the name of the source file that contains
//the line that detected the error, and ulLine holds the line number in the source
//file. The pcFile and ulLine values can be printed out, or otherwise recorded,
//before the following infinite loop is entered. */
//	printf("[ASSERT] %s: %d \r\n", pcFile, ulLine);
//
///* Disable interrupts so the tick interrupt stops executing, then sit in a loop
//so execution does not move past the line that failed the assertion. */
//	taskDISABLE_INTERRUPTS();
//	for( ;; );
//}

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
{
    volatile signed char *name;
    volatile xTaskHandle *pxT;

    name = pcTaskName;
    pxT  = pxTask;

    (void)name;
    (void)pxT;

    while(1);
}
#endif


/*-----------------------------------------------------------*/
/**
 * @brief	configASSERT callback function
 * @param 	ulLine		: line where configASSERT was called
 * @param 	pcFileName	: file where configASSERT was called
 */
void vAssertCalled(unsigned long ulLine, const char *const pcFileName)
{
	volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

	taskENTER_CRITICAL();
	{
		printf("[ASSERT] %s:%lu\n", pcFileName, ulLine);
		/* You can step out of this function to debug the assertion by using
		 the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
		 value. */
		while( ulSetToNonZeroInDebuggerToContinue == 0 )
		{
		}
	}
	taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/
