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

#define MOVING_AVERAGE_2N_SIZE	6

typedef struct _sample_t
{
    max3510x_time_t up;
    max3510x_time_t down;
}
sample_t;

static sample_t s_sample[1<<MOVING_AVERAGE_2N_SIZE];
static uint16_t s_sample_count;
static uint16_t s_sample_ndx;



float_t transducer_flow_compensate( float_t raw  )
{
    return raw;
}

static void add_sample( q31_t up, q31_t down )
{
    if( s_sample_ndx >= ARRAY_COUNT( s_sample ) )
        s_sample_ndx = 0;
    s_sample[s_sample_ndx].up = up;
    s_sample[s_sample_ndx].up = down;
    s_sample_ndx++;
    if( s_sample_count < ARRAY_COUNT( s_sample ) )
        s_sample_count++;
}


static bool get_flow( float_t * p_flow, float_t * p_sos )
{
    uint64_t up = 0;
    uint64_t down = 0;
    float_t delta;
    float_t prod;
    uint16_t i;
    bool r = false;
    /*
    if( sample_count < ARRAY_COUNT(s_sample) )
    {
        for(i=0;i<ARRAY_COUNT(s_sample))
        {
            up += s_sample[i].up;
            down += sample[i].down;
        }
        up >>= MOVING_AVERAGE_2N_SIZE;
        down >>= MOVING_AVERAGE_2N_SIZE;
        prod = (up * down);
        if( p_flow )
        {
            delta = (float_t)(up - down);
            *p_flow = (float_t)(int32_t)delta / prod;
        }
        if( p_sos )
        {
            sum =  (float_t)(up + down);
            *p_sos = (float_t)(uint32_t)sum / prod;
        }
        r = true;
    }
    */
    return r;
}


static void dir2fp( tdc_tof_fp_measurement_t * p_fp, const max3510x_measurement_t * p_native )
{
    uint32_t i;
    p_fp->average = max3510x_fixed_to_double( &p_native->average );
    p_fp->t1_ideal = p_native->t2_ideal;
    p_fp->t1_t2 = p_native->t1_t2;
    for( i = 0; i < ARRAY_COUNT( p_fp->hit ); i++ )
    {
        p_fp->hit[i] = max3510x_fixed_to_double( &p_native->hit[i] );
    }
}

static void native2fp( tdc_tof_fp_t * p_fp, const tdc_tof_native_t * p_native )
{
    dir2fp( &p_fp->up, &p_native->up );
    dir2fp( &p_fp->down, &p_native->down );
    //p_fp->tof_diff = max3510x_fixed_to_time( &p_native->tof_diff ) << FP_BIAS;
}


static uint32_t s_last_up;
static uint32_t s_last_down;
static uint32_t s_last_prod;

static uint64_t s_sum_up;
static uint64_t s_sum_down;
static uint64_t s_sum_prod;



static void task_flow( void * pv )
{
    static com_report_t report;
    static const tdc_tof_native_t * p_native = &report.sample.tof.native;
    static QueueSetMemberHandle_t qs;

    tdc_configure( config_load() );
    board_enable_sample_timer();

    while( 1 )
    {
        xSemaphoreTake( s_sample_clock_semaphore, portMAX_DELAY );
        tdc_cmd_tof_diff();
        xSemaphoreTake( s_sample_ready_semaphore, portMAX_DELAY );
        tdc_get_tof_result( &report.sample );
        if( (report.sample.status & MAX3510X_REG_INTERRUPT_STATUS_TO | MAX3510X_REG_INTERRUPT_STATUS_TOF) == MAX3510X_REG_INTERRUPT_STATUS_TOF )
        {
            transducer_compensated_tof( &s_last_prod, &s_last_up, &s_last_down, p_native->up.hit, p_native->down.hit );

            s_sum_up += s_last_up;
            s_sum_down += s_last_down;
            s_sum_prod += s_last_prod;

//			flow = transducer_flow_compensate( flow );

            //q31_t volume = transducer_volume( flow, )
            com_report( &report );
        }
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



