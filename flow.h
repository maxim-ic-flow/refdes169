#ifndef _FLOW_H_
#define _FLOW_H_

#include "max3510x.h"

void flow_init(void);

typedef struct _flow_sample_t
{
    max3510x_time_t up;
    max3510x_time_t down;
}
flow_sample_t;

void flow_sample_clock( void );
void flow_sample_complete(void);

uint32_t flow_get_temp_sampling_ratio(void);
void flow_set_temp_sampling_ratio( uint32_t count );
uint32_t flow_get_cal_sampling_ratio(void);
void flow_set_cal_sampling_ratio( uint32_t count );

void flow_set_sampling_frequency( uint8_t freq_hz );
uint8_t flow_get_sampling_frequency( void );

void flow_unlock( void );
void flow_lock( void );

void flow_set_down_offset( uint8_t up );
void flow_set_up_offset( uint8_t up );

void flow_set_ratio_tracking( float_t target );
float_t flow_get_ratio_tracking(void);

#endif