/*******************************************************************************
 * Copyright (C) 2018 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 ******************************************************************************/

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

    lcd_on();
    lcd_printf("MAXIM INTEGRATED"
               "  MAXREFDES169  "
               " Gas Flow Meter ");

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

