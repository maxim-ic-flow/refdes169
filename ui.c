#include "global.h"
#include "ui.h"


static void task_ui( void *pv )
{
    vTaskDelay( portMAX_DELAY );
}

void ui_init(void)
{
    xTaskCreate( task_ui , "ui", TASK_DEFAULT_STACK_SIZE, NULL, TASK_DEFAULT_PRIORITY, NULL );
}


