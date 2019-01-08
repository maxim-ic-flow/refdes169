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

#ifndef _FLOW_H_
#define _FLOW_H_

#include "max3510x.h"
#include "tdc.h"
#include "flowbody.h"

void flow_init(void);


void flow_sample_clock( void );
void flow_sample_complete( tdc_cmd_context_t cmd_context );

uint32_t flow_get_temp_sampling_ratio(void);
void flow_set_temp_sampling_ratio( uint32_t count );
uint32_t flow_get_cal_sampling_ratio(void);
void flow_set_cal_sampling_ratio( uint32_t count );

void flow_set_sampling_frequency( uint8_t freq_hz );
uint8_t flow_get_sampling_frequency( void );

typedef enum
{
    flow_state_init,
    flow_state_fatal,
	flow_state_idle,
	flow_state_connection,
    flow_state_running
}
flow_state_t;

flow_state_t flow_state( void );

bool flow_rate( flow_dt *p_flow );

void flow_set_down_offset( uint8_t up );
void flow_set_up_offset( uint8_t up );

void flow_set_ratio_tracking( uint16_t target );
void flow_set_minimum_offset( uint8_t offset_minimum );
uint8_t flow_get_minimum_offset( void );
uint16_t flow_get_ratio_tracking(void);

void flow_reset(void);

int32_t flow_zfo(void);
bool flow_temperature_ratio( double_t * p_temp_ratio );

#endif