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

static void tof_and_period( q31_t *p_toff, q31_t *p_period, const max3510x_fixed_t *p_hits )
{
    // returns metrics proportional to oscillation period and time-of-flight

    uint8_t i;
    q31_t sum ;
    q31_t h1, h2;
    q31_t diff = 0;

    sum = h1 = max3510x_fixed_to_time( p_hits );
    for(i=1;i<MAX3510X_MAX_HITCOUNT;i++)
    {
        h2 = max3510x_fixed_to_time( &p_hits[i] );
        sum += h2;
        diff += h2 - h1;
        h1 = h2;
    }
    *p_period = diff;
    *p_toff = sum;
}

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
            q31_t tof_up, period_up, tof_down, period_down;
            q31_t tof_up_inv, tof_down_inv;
            tof_and_period( &tof_up, &period_up, result.tof_result.tof.up.hit );
            tof_and_period( &tof_down, &period_down, result.tof_result.tof.down.hit );
            
            tof_up <<= 6;
            tof_down <<= 6;
             
            arm_recip_q31( tof_up, &tof_up_inv, armRecipTableQ31 );
            arm_recip_q31( tof_down, &tof_down_inv, armRecipTableQ31 );
            
            q31_t flow2 = tof_down_inv - tof_up_inv;  // this quantity is proportional to linear and volumetric flow
            
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
