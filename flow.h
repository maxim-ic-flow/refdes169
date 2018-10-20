#ifndef _FLOW_H_
#define _FLOW_H_

#include "max3510x.h"

void flow_init(void);


void flow_sample_clock( void );
void flow_sample_complete(void);

uint32_t flow_get_temp_sampling_ratio(void);
void flow_set_temp_sampling_ratio( uint32_t count );
uint32_t flow_get_cal_sampling_ratio(void);
void flow_set_cal_sampling_ratio( uint32_t count );

void flow_set_sampling_frequency( uint8_t freq_hz );
uint8_t flow_get_sampling_frequency( void );
void flow_set_zero_flow( max3510x_time_t offset );

void flow_unlock( void );
void flow_lock( void );

void flow_set_down_offset( uint8_t up );
void flow_set_up_offset( uint8_t up );

void flow_set_ratio_tracking( uint16_t target );
void flow_set_minimum_offset( uint8_t offset_minimum );
uint8_t flow_get_minimum_offset( void );
uint16_t flow_get_ratio_tracking(void);

void flow_reset(void);

#endif