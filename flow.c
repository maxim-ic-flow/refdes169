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
#include "flow.h"
#include "board.h"
#include "flowbody.h"
#include "com.h"
#include "config.h"
#include "tdc.h"
#include "wave_track.h"

typedef struct _flow_meter_t
{
    int32_t *   flow;
    int32_t *   sos;
    int64_t     volumetric;
    uint16_t    count;
    uint16_t    ndx;
}
flow_meter_t;

typedef enum
{
    event_id_clock,
    event_id_interrupt,
    event_id_sample_rate
}
event_id_t;

typedef struct
{
    event_id_t  event_id;
    uint32_t    param;
}
event_t;

static QueueHandle_t            s_event_queue;
static bool                     s_interactive;
static uint8_t                  s_offset_up_pending;
static uint8_t                  s_offset_down_pending;
static wave_track_direction_t   s_up, s_down;
static uint32_t                 s_sample_count;
static flow_dt                  s_flow_accumulator;
static max3510x_time_t          s_zero_flow_offset;

void flow_sample_complete( tdc_cmd_context_t cmd_context )
{
    // ISR context
    static BaseType_t woken = pdFALSE;
    static event_t event =
    {
        .event_id = event_id_interrupt
    };
    event.param = (uint32_t)cmd_context;
    xQueueSendFromISR( s_event_queue, &event, &woken  );
    portYIELD_FROM_ISR( woken );
}

void flow_sample_clock( void )
{
    // ISR context
    // WARNING:  breakpoints in this function will break the sample clock
    static BaseType_t woken = pdFALSE;
    static const event_t event =
    {
        .event_id = event_id_clock
    };
    xQueueSendFromISR( s_event_queue, &event, &woken  );
    portYIELD_FROM_ISR( woken );
}


static uint32_t s_ratio_counter;


typedef struct _counter_t
{
    uint32_t    reload;
    uint32_t    current;
}
counter_t;

static counter_t s_temperature_counter;
static counter_t s_calibration_counter;

static void set_counter( counter_t * p_counter, uint32_t count )
{
    vTaskSuspendAll();
    p_counter->reload = count;
    p_counter->current = s_ratio_counter;
    xTaskResumeAll();
}

static bool counter( counter_t * p_counter )
{
    vTaskSuspendAll();
    if( p_counter->reload && (p_counter->current == s_ratio_counter) )
    {
        p_counter->current += p_counter->reload;
        xTaskResumeAll();
        return true;
    }
    xTaskResumeAll();
    return false;
}

uint8_t flow_get_minimum_offset( void )
{
    return s_up.mimimum_offset;
}

void flow_set_minimum_offset( uint8_t offset_minimum )
{
    s_up.mimimum_offset = offset_minimum;
    s_down.mimimum_offset = offset_minimum;
}

void flow_set_ratio_tracking( uint16_t target )
{
    s_up.ratio_tracking = target;
    s_down.ratio_tracking = target;
}

uint16_t flow_get_ratio_tracking( void )
{
    return s_up.ratio_tracking;
}

void flow_reset( void )
{
    // reset volumetric measurements
    s_ratio_counter = 0;
    s_calibration_counter.current = 0;
    s_temperature_counter.current = 0;
}


void flow_set_up_offset( uint8_t up )
{
    s_offset_up_pending = up;
}

void flow_set_down_offset( uint8_t down )
{
    s_offset_down_pending = down;
}

static void process_tdc_measurement( tdc_cmd_context_t cmd_context )
{
    static tdc_result_t         result;
    static flowbody_sample_t    sample;
    static com_meter_t          meter;


    static bool calibration_defered = false;
    if( s_interactive )
    {
        com_interactive_report_t report;
        tdc_get_tof_result( &report.tof );
        report.cmd_context = cmd_context;
        com_report( report_type_native, (const com_report_t*)&report );
        return;
    }
    if( cmd_context == tdc_cmd_context_tof_diff  )
    {
        tdc_get_tof_result( &result.tof );
        if( counter( &s_temperature_counter ) )
        {
            tdc_cmd_temperature();
            if( counter( &s_calibration_counter ) )
                calibration_defered = true;
            else
                calibration_defered = false;
        }
        else if( counter( &s_calibration_counter ) )
        {
            calibration_defered = false;
            tdc_cmd_calibrate();
        }
        bool tracking = false;
        board_tot( board_tot_state_toggle );
        if( (result.status & (MAX3510X_REG_INTERRUPT_STATUS_TO | MAX3510X_REG_INTERRUPT_STATUS_TOF)) == MAX3510X_REG_INTERRUPT_STATUS_TOF )
        {
            bool up_tracked, down_tracked;
            up_tracked = wave_track_direction( &s_up, result.tof.tof.up.hit, result.tof.tof.up.t1_t2 );
            down_tracked = wave_track_direction( &s_down, result.tof.tof.down.hit, result.tof.tof.down.t1_t2 );
            if( up_tracked && down_tracked )
            {
                if( !tracking )
                    tracking = wave_track_converge( &s_up, &s_down );

            }
        }
        // TOF data is validated
        // Convert time-of-flight values to flow and speed-of-sound
        if( tracking )
        {
            board_led( BOARD_LED_RED, board_led_state_off );
            flow_dt flow, sos;
            sample.up = s_up.tof;
            sample.down = s_down.tof;
            sample.up_period = s_up.period;
            sample.down_period = s_down.period;
            max3510x_time_t up, down;
            flowbody_transducer_compensate( &sample, &up, &down );
            flowbody_flow_sos( up, down, &flow, &sos );

            s_sample_count++;
            s_flow_accumulator += flow;
            meter.volumetric = s_flow_accumulator;
            meter.sos = sos;
            meter.flow = flow;
        }
        else
        {
            // all zero's indicates that the measurement timed out or couldn't be tracked.
            board_led( BOARD_LED_RED, board_led_state_on );
            memset( &sample, 0, sizeof(sample) );
            memset( &result.tof.tof, 0, sizeof(result.tof.tof) );
            memset( &meter, 0, sizeof(meter) );
        }
        com_report( report_type_tracked, (const com_report_t*)&sample );
        com_report( report_type_detail, (const com_report_t*)&result );
        com_report( report_type_meter, (const com_report_t*)&meter );
        s_ratio_counter++;
    }
    else if( cmd_context == tdc_cmd_context_temperature )
    {
        tdc_get_temperature_result( &result.temperature );
        if( calibration_defered )
        {
            calibration_defered = false;
            tdc_cmd_calibrate();
        }
        if( (result.status & (MAX3510X_REG_INTERRUPT_STATUS_TO | MAX3510X_REG_INTERRUPT_STATUS_TE)) == MAX3510X_REG_INTERRUPT_STATUS_TE )
        {
            uint32_t t1 = max3510x_fixed_to_time( &result.temperature.temperature[0] );
            uint32_t t2 = max3510x_fixed_to_time( &result.temperature.temperature[1] );
            if( t1 != 0xFFFFFFFF && t2 != 0xFFFFFFFF )
            {
                // valid temperature sample
            }
        }
        else
        {
            memset( &result.temperature.temperature, 0, sizeof(result.temperature.temperature) );
        }
        com_report( report_type_detail, (const com_report_t*)&result );
    }
    else if( cmd_context == tdc_cmd_context_calibrate )
    {
        tdc_get_calibration_result( &result.calibration );
        com_report( report_type_detail, (const com_report_t*)&result );
    }
}

static void task_flow( void * pv )
{
    bool                        do_temperature;
    bool                        tracking;

    static QueueSetMemberHandle_t qs;
    static event_t              event;

    const config_t * p_config = config_load();
    tdc_configure( &p_config->chip );
    flow_set_temp_sampling_ratio( p_config->algo.temperature_ratio );
    flow_set_cal_sampling_ratio( p_config->algo.calibration_ratio );
    flow_set_ratio_tracking( p_config->algo.ratio_tracking );
    flow_set_minimum_offset( p_config->algo.offset_minimum );
    tdc_read_thresholds( &s_up.comparator_offset, &s_down.comparator_offset );
    board_set_sampling_frequency( p_config->algo.sampling_frequency );


    while( 1 )
    {
        xQueueReceive( s_event_queue, &event, portMAX_DELAY );
        switch (event.event_id)
        {
            case event_id_clock:
            {
                if( s_interactive )
                    continue;
                if( s_offset_up_pending )
                {
                    s_up.comparator_offset = s_offset_up_pending;
                    s_offset_up_pending = 0;
                }
                if( s_offset_down_pending )
                {
                    s_down.comparator_offset = s_offset_down_pending;
                    s_offset_down_pending = 0;
                }
                tdc_adjust_and_measure( s_up.comparator_offset, s_down.comparator_offset );
                break;
            }
            case event_id_interrupt:
            {
                process_tdc_measurement( (tdc_cmd_context_t)event.param );
                break;
            }
            case event_id_sample_rate:
            {
                uint8_t hz = (uint8_t)event.param;
                board_set_sampling_frequency( hz );
                s_interactive = hz ? false : true;
                break;
            }
        }
    }
}


void flow_init( void )
{
    s_event_queue = xQueueCreate( 4, sizeof(event_t) );
    configASSERT( s_event_queue );
    xTaskCreate( task_flow, "flow", TASK_DEFAULT_STACK_SIZE, NULL, TASK_DEFAULT_PRIORITY, NULL );
}

uint32_t flow_get_temp_sampling_ratio( void )
{
    return s_temperature_counter.reload;
}

void flow_set_temp_sampling_ratio( uint32_t count )
{
    set_counter( &s_temperature_counter, count );
}

uint32_t flow_get_cal_sampling_ratio( void )
{
    return s_calibration_counter.reload;
}

void flow_set_cal_sampling_ratio( uint32_t count )
{
    set_counter( &s_calibration_counter, count );
}

void flow_set_sampling_frequency( uint8_t freq_hz )
{
    static event_t event =
    {
        .event_id = event_id_sample_rate
    };
    event.param = (uint32_t)freq_hz;
    xQueueSend( s_event_queue, &event, portMAX_DELAY );
}

uint8_t flow_get_sampling_frequency( void )
{
    return  board_get_sampling_frequency();
}

