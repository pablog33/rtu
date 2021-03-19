#include <stdbool.h>

#include "board.h"


/**
 * @brief 	initializes DOUTs
 * @return	nothing
 */
void din_init()
{
	Chip_SCU_PinMuxSet( 7, 6, SCU_MODE_FUNC4 );			//DOUT4 P7_6 	PIN134  	GPIO3[14]   WDT_RST_TEST
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 3, 14);
}




