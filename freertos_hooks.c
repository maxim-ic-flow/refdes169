#include "global.h"

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;
	taskDISABLE_INTERRUPTS();
	while(1);
}

void vApplicationMallocFailedHook(void)
{
	taskDISABLE_INTERRUPTS();
    while(1);
}


