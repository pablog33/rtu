#include "debug.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

enum debugLevels debugLevel = Info;

FILE *debugFile = NULL;

SemaphoreHandle_t uart_mutex;

void debugInit(void)
{
	uart_mutex = xSemaphoreCreateMutex();
}

/**
 * @brief 	sets debug level.
 * @param 	lvl 	:name of file to send output to
 */
void debugSetLevel(enum debugLevels lvl)
{
	debugLevel = lvl;
}

/**
 * @brief sends debugging output to a file.
 * @param fileName name of file to send output to
 */
void debugToFile(const char *fileName)
{
	debugClose();

	FILE *f = fopen(fileName, "w"); // "w+" ?

	if (f)
		debugFile = f;
}

/** Close the output file if it was set in <tt>toFile()</tt> */
void debugClose(void)
{
	if (debugFile && (debugFile != stderr)) {
		fclose(debugFile);
		debugFile = stderr;
	}
}
