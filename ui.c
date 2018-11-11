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
#include "flow.h"

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

static float_t resistance2celicus( double_t resistance )
{
    const double_t r0 = 1000;
    const double_t a = 3.9083e-3;
    const double_t b = -5.775e-7;
    const double_t c1 = 4 * r0 * b;
    const double_t c2 = a*a*r0*r0;
    const double_t c3 = -a * r0;
    const double_t c4 = 2 * r0 * b;

    double_t dr = r0 - resistance;
    double_t rad = sqrt( c2 - c1 * dr);
    return (( c3 + rad) / c4 );
}

static void task_ui( void *pv )
{
    lcd_on();
    lcd_printf("MAXIM INTEGRATED"
               "  MAXREFDES169  "
               "                ");

    vTaskDelay(portDELAY_MS(3000));

    while( 1 )
    {
        if( pdTRUE == xSemaphoreTake( s_semaphore, portDELAY_MS(LCD_UPDATE_PERIOD_MS) ) )
        {
            // button event
            vTaskDelay(portDELAY_MS(1));
            if( board_buttons() == s_button_state  )
            {
                // debounced
            }
            board_buttons_enable(true);
        }
        else
        {
            double_t temperature_ratio;
            float_t flow = flow_rate();
            if( flow_temperature_ratio( &temperature_ratio ) )
            {
                float_t temp = resistance2celicus( 1000.0f * temperature_ratio );
                lcd_printf( "Flow:      %5.2f"
                            "Temp:      %5.1f"
                            "                ", flow, temp );
            }
            else
            {
                lcd_printf( "Flow:      %2.2f"
                           "                "
                           "                ", flow );
            }
        }
    }
}

void ui_init(void)
{
    s_semaphore = xSemaphoreCreateBinary();
    xTaskCreate( task_ui , "ui", TASK_DEFAULT_STACK_SIZE, NULL, TASK_DEFAULT_PRIORITY, NULL );
}

