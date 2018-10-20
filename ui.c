#include "global.h"
#include "ui.h"
#include "board.h"

#define MAX_BUTTON_EVENTS 2

typedef struct _button_event_t
{
	board_button_t	button;
	bool			state;
}
button_event_t;

static QueueHandle_t s_button_queue;

void ui_button ( board_button_t button, bool state )
{
    BaseType_t woken = pdFALSE;
    button_event_t event;
    event.button = button;
    event.state = state;
    xQueueSendToBackFromISR( s_button_queue, &event, &woken );
    portYIELD_FROM_ISR( woken );
}

static void task_ui( void *pv )
{
    button_event_t event;
    bool lcd_power_state;

    while( 1 )
    {
        xQueueReceive( s_button_queue, &event, portMAX_DELAY );
        switch( event.button )
        {
            case board_button_escape:
            {
                if( event.state )
                {
                    lcd_power_state = !lcd_power_state;
                    if( lcd_power_state )
                        lcd_on();
                    else
                        lcd_off();
                }
                break;
            }
        }
    }
}

void ui_init(void)
{
    s_button_queue = xQueueCreate( MAX_BUTTON_EVENTS, sizeof(button_event_t) );
    xTaskCreate( task_ui , "ui", TASK_DEFAULT_STACK_SIZE, NULL, TASK_DEFAULT_PRIORITY, NULL );
}


