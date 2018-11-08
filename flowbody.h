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

#ifndef _flowbody_H_
#define _flowbody_H_

#include "max3510x.h"
#include "config.h"


typedef struct _flowbody_sample_t
{
    max3510x_time_t up;
    max3510x_time_t down;
    max3510x_time_t up_period;
    max3510x_time_t down_period;
}
flowbody_sample_t;

typedef float_t flow_dt;

void flowbody_flow_sos( max3510x_time_t up, max3510x_time_t down, flow_dt *p_flow, flow_dt *p_sos );

bool flowbody_config( config_t *p_config, uint8_t ndx );
void flowbody_transducer_compensate( const flowbody_sample_t *p_sample, uint32_t * p_up, uint32_t * p_down );

flow_dt flowbody_volumetric( flow_dt integrated_flow, uint32_t sample_count );
void flowbody_linearize( flow_dt *p_flow, flow_dt flow, flow_dt *p_sos, flow_dt sos );
float_t flowbody_hmi_flow( flow_dt flow );
float_t flowbody_hmi_sos( flow_dt sos );
float_t flowbody_hmi_volumetric( int64_t volumetric );

#endif

