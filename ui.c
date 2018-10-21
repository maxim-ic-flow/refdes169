#include "global.h"
#include "ui.h"
#include "board.h"
#include "lcd.h"

#define MAX_BUTTON_EVENTS 2

#define LCD_UPDATE_PERIOD_MS   1000
#define LCD_POWER_OFF_DELAY_MS 3000

static QueueHandle_t s_semaphore;
static uint32_t s_button_state;

void ui_buttons_isr(void)
{
    BaseType_t woken = pdFALSE;
    s_button_state = board_buttons();
    board_buttons_enable(false);
    xSemaphoreGiveFromISR( s_semaphore, &woken );
    portYIELD_FROM_ISR( woken );
}

static void task_ui( void *pv )
{
    static const TickType_t lcd_delay[2] = { portMAX_DELAY, portDELAY_MS(LCD_UPDATE_PERIOD_MS) };
    
    uint8_t lcd_off_count;
    bool lcd_power_state;
    uint32_t last_button_state = board_buttons();

    while( 1 )
    {
        if( pdTRUE == xSemaphoreTake( s_semaphore, lcd_delay[lcd_power_state] ) )
        {
            // button event
            vTaskDelay(portDELAY_MS(1));
            if( board_buttons() == s_button_state  )
            {
                // debounced
                if( !last_button_state && s_button_state )
                {
                    // one of the four user buttons has been pressed.
                    if( lcd_power_state )
                    {
                    }
                    else
                    {
                        //   power up the LCD.
                        lcd_on();
//                        lcd_cursor(0,0);
//                        const char *x = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
//                        const char *y = "************************************************";
                        //lcd_write_data(x, strlen(x));
                        //lcd_write_data(y, strlen(y));
                        lcd_power_state = true;
                        lcd_off_count = LCD_POWER_OFF_DELAY_MS/LCD_UPDATE_PERIOD_MS;
                    }
                }
                last_button_state = s_button_state;
            }
            board_buttons_enable(true);
        }
        else
        {
            // time out
            if( lcd_power_state && lcd_off_count && !--lcd_off_count)
            {
                lcd_off();
                lcd_power_state = false;
            }
        }
    }
}

void ui_init(void)
{
    s_semaphore = xSemaphoreCreateBinary();
    xTaskCreate( task_ui , "ui", TASK_DEFAULT_STACK_SIZE, NULL, TASK_DEFAULT_PRIORITY, NULL );
}


