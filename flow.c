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
    static BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR( s_sample_clock_semaphore, &woken );
    portYIELD_FROM_ISR( woken );
}

static uint32_t s_last_up;
static uint32_t s_last_down;
static uint32_t s_last_prod;

static uint64_t s_sum_up;
static uint64_t s_sum_down;
static uint64_t s_sum_prod;
static uint16_t	s_temp_prescaler;

#define TOFS_PER_TEMP	5


static void task_flow( void * pv )
{
    static tdc_result_t result; 
    static QueueSetMemberHandle_t qs;

    tdc_configure( config_load() );
    board_enable_sample_timer();

    while( 1 )
    {
        xSemaphoreTake( s_sample_clock_semaphore, portMAX_DELAY );

        tdc_cmd_tof_diff();
        xSemaphoreTake( s_sample_ready_semaphore, portMAX_DELAY );
		if( !s_temp_prescaler )
			tdc_cmd_temperature();
		tdc_get_tof_result( &result.tof_result );
		if( (result.status & (MAX3510X_REG_INTERRUPT_STATUS_TO|MAX3510X_REG_INTERRUPT_STATUS_TOF)) == MAX3510X_REG_INTERRUPT_STATUS_TOF)
		{
			transducer_compensated_tof( &s_last_prod, &s_last_up, &s_last_down, result.tof_result.tof.up.hit, result.tof_result.tof.down.hit );
            s_sum_up += s_last_up;
			s_sum_down += s_last_down;
			s_sum_prod += s_last_prod;

//			flow = transducer_flow_compensate( flow );

            //q31_t volume = transducer_volume( flow, )
        }
		if( !s_temp_prescaler )
		{
			xSemaphoreTake( s_sample_ready_semaphore, portMAX_DELAY );
			if( (result.status & (MAX3510X_REG_INTERRUPT_STATUS_TO|MAX3510X_REG_INTERRUPT_STATUS_TE)) == MAX3510X_REG_INTERRUPT_STATUS_TE)
			{
				uint32_t t1 = max3510x_fixed_to_time( &result.temperature_result.temperature[0] );
				uint32_t t2 = max3510x_fixed_to_time( &result.temperature_result.temperature[1] );

			}
		}
		if( ++s_temp_prescaler >= TOFS_PER_TEMP )
			s_temp_prescaler = 0;
		com_report( &result );
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



