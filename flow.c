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
	int32_t * 	flow;
	int32_t * 	sos;
	int64_t		volumetric;
	uint16_t	count;
	uint16_t	ndx;
}
flow_meter_t;


static SemaphoreHandle_t        s_sample_clock_semaphore;
static SemaphoreHandle_t        s_sample_ready_semaphore;
static SemaphoreHandle_t        s_flow_semaphore;
static bool                     s_sampling;
static uint8_t 					s_offset_up_pending;
static uint8_t 					s_offset_down_pending;
static wave_track_direction_t 	s_up, s_down;
static uint32_t					s_sample_count;
static flow_dt					s_flow_accumulator;
static bool						s_tracking;

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


static uint32_t s_ratio_counter;


typedef struct _counter_t
{
	uint32_t	reload;
	uint32_t	current;
}
counter_t;

static counter_t s_temperature_counter;
static counter_t s_calibration_counter;

static void set_counter( counter_t *p_counter, uint32_t count )
{
	vTaskSuspendAll();
    p_counter->reload = count;
    p_counter->current = s_ratio_counter;
	xTaskResumeAll();
}

static bool counter( counter_t *p_counter )
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

static void interactive_mode( void )
{
	com_report_t report;
	tdc_cmd_context_t cmd_context;
	while( !s_sampling )
	{
		xSemaphoreTake( s_sample_ready_semaphore, portMAX_DELAY );
        if( s_sampling )
            break;
		switch( cmd_context = tdc_cmd_context() )
		{
			case tdc_cmd_context_tof_diff:
            case tdc_cmd_context_tof_up:
            case tdc_cmd_context_tof_down:
				tdc_get_tof_result( &report.interactive.tof );
				report.interactive.cmd_context = cmd_context;
				com_report( report_type_native, (const com_report_t*)&report );
				break;
		}

	}
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

uint16_t flow_get_ratio_tracking(void)
{
	return s_up.ratio_tracking;
}


void flow_set_up_offset( uint8_t up )
{
	s_offset_up_pending = up;
}

void flow_set_down_offset( uint8_t down )
{
	s_offset_down_pending = down;
}

static void task_flow( void * pv )
{
	bool do_temperature;
    static tdc_result_t 		result;

    static QueueSetMemberHandle_t qs;

    const config_t *p_config = config_load();
    
    tdc_configure( &p_config->chip );

    flow_set_temp_sampling_ratio(p_config->algo.temperature_ratio);
    flow_set_cal_sampling_ratio(p_config->algo.calibration_ratio);
    flow_set_sampling_frequency(p_config->algo.sampling_frequency);
	flow_set_ratio_tracking(p_config->algo.ratio_tracking);
	flow_set_minimum_offset(p_config->algo.offset_minimum);
	board_set_squelch_time(p_config->algo.squelch);

	tdc_read_thresholds( &s_up.comparator_offset, &s_down.comparator_offset );
	
    while( 1 )
    {
		if( s_sampling )
		{
            xSemaphoreTake( s_sample_clock_semaphore, portMAX_DELAY );
		}
		if( !s_sampling )
		{
			interactive_mode();
			tdc_read_thresholds( &s_up.comparator_offset, &s_down.comparator_offset );
			continue;
		}
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
        tdc_measure(s_up.comparator_offset, s_down.comparator_offset);

		flow_lock();
        xSemaphoreTake( s_sample_ready_semaphore, portMAX_DELAY );
		if( do_temperature = counter( &s_temperature_counter ))
		{
            tdc_cmd_temperature();
		}
        tdc_get_tof_result( &result.tof );
        if( (result.status & (MAX3510X_REG_INTERRUPT_STATUS_TO | MAX3510X_REG_INTERRUPT_STATUS_TOF)) == MAX3510X_REG_INTERRUPT_STATUS_TOF )
        {
			bool up_tracked, down_tracked;

			if( up_tracked = wave_track_direction( &s_up, result.tof.tof.up.hit, result.tof.tof.up.t1_t2 ) )
			{
				board_led(0,false);
			}
			else
			{
				board_led(0,true);
			}
			if( down_tracked = wave_track_direction( &s_down, result.tof.tof.down.hit, result.tof.tof.down.t1_t2 ) )
			{
				board_led(1,false);
			}
			else
			{ 
				board_led(1,true);
			}
			if( up_tracked && down_tracked )
			{
				if( !s_tracking )
					s_tracking = wave_track_converge( &s_up, &s_down );

				// TOF data is validated
                // Convert time-of-flight values to flow and speed-of-sound
				if( s_tracking )
				{
					static flowbody_sample_t 	sample;
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
					static com_meter_t meter;
					meter.volumetric = s_flow_accumulator;
					meter.sos = sos;
					meter.flow = flow;
					com_report( report_type_tracked, (const com_report_t *)&sample );
					com_report(report_type_detail, (const com_report_t*)&result );
					com_report(report_type_meter, (const com_report_t*)&meter );
				}
			}
        }
        if( do_temperature )
        {
            xSemaphoreTake( s_sample_ready_semaphore, portMAX_DELAY );
            tdc_get_temperature_result( &result.temperature );

            if( (result.status & (MAX3510X_REG_INTERRUPT_STATUS_TO | MAX3510X_REG_INTERRUPT_STATUS_TE)) == MAX3510X_REG_INTERRUPT_STATUS_TE )
            {
                uint32_t t1 = max3510x_fixed_to_time( &result.temperature.temperature[0] );
                uint32_t t2 = max3510x_fixed_to_time( &result.temperature.temperature[1] );
				if( t1 != 0xFFFFFFFF && t2 != 0xFFFFFFFF )
				{
					// valid temperature sample
				}
				com_report( report_type_detail, (const com_report_t *)&result );
            }
        }
		if( counter( &s_calibration_counter ) )
		{
            tdc_cmd_calibrate();
            xSemaphoreTake( s_sample_ready_semaphore, portMAX_DELAY );
			tdc_get_calibration_result( &result.calibration );
            while(!(result.status & MAX3510X_REG_INTERRUPT_STATUS_CAL ));
			com_report(report_type_detail, (const com_report_t*)&result );
		}
		s_ratio_counter++;
   		flow_unlock();
    }
}

void flow_lock( void )
{
    xSemaphoreTakeRecursive( s_flow_semaphore, portMAX_DELAY );
}

void flow_unlock( void )
{
    xSemaphoreGiveRecursive( s_flow_semaphore );
}

void flow_init( void )
{
    s_sample_clock_semaphore = xSemaphoreCreateBinary();
    configASSERT( s_sample_clock_semaphore );
    s_sample_ready_semaphore = xSemaphoreCreateBinary();
    configASSERT( s_sample_ready_semaphore );
    s_flow_semaphore = xSemaphoreCreateRecursiveMutex();
    configASSERT( s_flow_semaphore );
    xTaskCreate( task_flow, "flow", TASK_DEFAULT_STACK_SIZE, NULL, TASK_DEFAULT_PRIORITY, NULL );
}

uint32_t flow_get_temp_sampling_ratio(void)
{
    return s_temperature_counter.reload;
}

void flow_set_temp_sampling_ratio( uint32_t count )
{
	set_counter( &s_temperature_counter, count );
}

uint32_t flow_get_cal_sampling_ratio(void)
{
    return s_calibration_counter.reload;
}

void flow_set_cal_sampling_ratio( uint32_t count )
{
	set_counter( &s_calibration_counter, count );
}

void flow_set_sampling_frequency( uint8_t freq_hz )
{
    board_set_sampling_frequency(freq_hz);
    s_sampling = freq_hz > 0 ? true : false;
    if( s_sampling )
		xSemaphoreGive( s_sample_ready_semaphore );
	else
        xSemaphoreGive( s_sample_clock_semaphore );
}

uint8_t flow_get_sampling_frequency( void )
{
    return  board_get_sampling_frequency();
}

