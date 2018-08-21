#ifndef _FLOW_H_
#define _FLOW_H_

#include "arm_math.h"

void flow_init(void);

typedef struct _flow_sample_t
{
    q31_t up;
    q31_t down;
    q31_t product;
}
flow_sample_t;

typedef struct _flow_accmulation_t
{
    flow_sample_t   last;
    q63_t           up;
    q63_t           down;
    q63_t           product;
}
flow_accmulation_t;

void flow_sample_clock( void );
void flow_sample_complete(void);
uint16_t flow_get_temp_sampling_ratio(void);
void flow_set_temp_sampling_ratio( uint16_t ratio );

#endif