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

#ifndef _WAVE_TRACK_H_
#define _WAVE_TRACK_H_

#include <math.h>
#include "max3510x.h"

#define WAVE_TRACK_HIT_COUNTS	MAX3510X_MAX_HITCOUNT		// must be > 2 && < MAX3510X_MAX_HITCOUNT

typedef struct _wave_track_direction_t
{
	max3510x_time_t period;			// on return is the average waveform period.
	max3510x_time_t tof;			//  on call is the last TOF sample.  on return is the new TOF sample
	max3510x_time_t hits[WAVE_TRACK_HIT_COUNTS];  // on return is all hit wave TOF's in 32-bit integer format
	uint16_t ratio_tracking;		// Q16 fraction of T1 wave peak that identifies the ideal comparator threshold.
									// 0.707 (0xB4FE) is a good place to start.  Set to zero to disable amplitude tracking.
	uint8_t	comparator_offset;		// on call is the current comparator offset.  on return is the recommended comparator offset
	uint8_t	 mimimum_offset;		// used to limit the comparator threshold to always be above noise.
	int8_t	phase;					// used to determine which of hits[] is considered TOF
}
wave_track_direction_t;




uint8_t wave_track_amplitude( uint8_t offset, uint8_t t1_t2, uint16_t target );
uint16_t wave_track_linearize_ratio( uint8_t ratio );
int8_t wave_track_phase( max3510x_time_t ref, max3510x_time_t sample, max3510x_time_t period );
bool wave_track_direction( wave_track_direction_t * p_dir, const max3510x_fixed_t *p_hits, uint8_t t1_t2_ratio );
max3510x_time_t wave_track_process_sample( max3510x_time_t *p_tof, const max3510x_fixed_t *p_hits );
bool wave_track_converge( wave_track_direction_t * p_up, wave_track_direction_t * p_down );

#endif

