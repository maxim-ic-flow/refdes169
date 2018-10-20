#include "global.h"
#include "board.h"
#include "com.h"
#include "flow.h"
#include "tdc.h"
#include "ui.h"


void main( void )
{
    board_init();

    ui_init();
    com_init();
    tdc_init();
    flow_init();
    vTaskStartScheduler();
}
