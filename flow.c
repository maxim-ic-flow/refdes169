#include "global.h"
#include "flow.h"
#include "board.h"
#include "transducer.h"
#include "com.h"
#include "config.h"
#include "tdc.h"
#include "arm_math.h"
#include "arm_common_tables.h"

static SemaphoreHandle_t        s_sample_clock_semaphore;
static SemaphoreHandle_t        s_sample_ready_semaphore;

void flow_sample_complete( void )
{
    // ISR context
    static BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR( s_sample_ready_semaphore, &woken );
    portYIELD_FROM_ISR( woken );
}

void flow_sample_clock( void )
{
    // ISR context
    // WARNING:  breakpoints in this function will break the sample clock
    static BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR( s_sample_clock_semaphore, &woken );
    portYIELD_FROM_ISR( woken );
}


static uint16_t s_temp_prescaler;
static uint16_t s_temp_ratio;

static void task_flow( void * pv )
{
    static tdc_result_t result;
    static flow_accmulation_t acc;
    static QueueSetMemberHandle_t qs;

    tdc_configure( config_load() );
    board_set_sampling_frequency(1);
    board_enable_sample_timer();

    while( 1 )
    {
        xSemaphoreTake( s_sample_clock_semaphore, portMAX_DELAY );

        tdc_cmd_tof_diff();
        xSemaphoreTake( s_sample_ready_semaphore, portMAX_DELAY );
        if( !s_temp_prescaler )
            tdc_cmd_temperature();
        tdc_get_tof_result( &result.tof_result );
        if( (result.status & (MAX3510X_REG_INTERRUPT_STATUS_TO | MAX3510X_REG_INTERRUPT_STATUS_TOF)) == MAX3510X_REG_INTERRUPT_STATUS_TOF )
        {
            transducer_compensated_tof( &acc.last.product, &acc.last.up, &acc.last.down, result.tof_result.tof.up.hit, result.tof_result.tof.down.hit );

            acc.up += acc.last.up;
            acc.down += acc.last.down;
            acc.product += acc.last.product;

//			flow = transducer_flow_compensate( flow );

            //q31_t volume = transducer_volume( flow, )
            com_report(report_type_raw, (const com_report_t*)&result );
            com_report(report_type_fixed, (const com_report_t*)&acc );
        }
        if( !s_temp_prescaler )
        {
            xSemaphoreTake( s_sample_ready_semaphore, portMAX_DELAY );
            tdc_get_temperature_result( &result.temperature_result );

            if( (result.status & (MAX3510X_REG_INTERRUPT_STATUS_TO | MAX3510X_REG_INTERRUPT_STATUS_TE)) == MAX3510X_REG_INTERRUPT_STATUS_TE )
            {
                uint32_t t1 = max3510x_fixed_to_time( &result.temperature_result.temperature[0] );
                uint32_t t2 = max3510x_fixed_to_time( &result.temperature_result.temperature[1] );

            }
            com_report( report_type_raw, (const com_report_t*)&result );
        }
        if( ++s_temp_prescaler >= s_temp_ratio )
            s_temp_prescaler = 0;
    }
}


void flow_init( void )
{
    s_sample_clock_semaphore = xSemaphoreCreateBinary();
    configASSERT( s_sample_clock_semaphore );
    s_sample_ready_semaphore = xSemaphoreCreateBinary();
    configASSERT( s_sample_ready_semaphore );

    xTaskCreate( task_flow, "flow", TASK_DEFAULT_STACK_SIZE, NULL, TASK_DEFAULT_PRIORITY, NULL );
}

uint16_t flow_get_temp_sampling_ratio(void)
{
    return s_temp_ratio;
}

void flow_set_temp_sampling_ratio( uint16_t ratio )
{
    s_temp_ratio = ratio;
}
