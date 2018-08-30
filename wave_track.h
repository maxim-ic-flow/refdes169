#ifndef _WAVE_TRACK_H_
#define _WAVE_TRACK_H_

#include <math.h>
#include "max3510x.h"

#define WAVE_TRACK_HIT_COUNTS	MAX3510X_MAX_HITCOUNT		// must be > 2 && < MAX3510X_MAX_HITCOUNT

typedef struct _wave_track_t
{
	max3510x_time_t period;			// on return is the average waveform period.
	max3510x_time_t tof;			//  on call is the last TOF sample.  on return is the new TOF sample
	max3510x_time_t hits[WAVE_TRACK_HIT_COUNTS];  // on return is all hit wave TOF's in 32-bit integer format
	uint16_t amplitude_target;		// Q16 normalized fraction of T1 wave peak that identifies the ideal comparator threshold
	uint8_t	comparator_offset;		// on call is the current comparator offset.  on return is the recommended comparator offset
}
wave_track_t;

uint8_t wave_track_amplitude( uint8_t offset, uint8_t t1_t2, uint16_t target );
uint16_t wave_track_linearize_ratio( uint8_t ratio );
int8_t wave_track_phase( max3510x_time_t ref, max3510x_time_t sample, max3510x_time_t period );
bool wave_track( wave_track_t * p_wt, const max3510x_fixed_t *p_hits, uint8_t t1_t2_ratio );
max3510x_time_t wave_track_process_sample( max3510x_time_t *p_tof, const max3510x_fixed_t *p_hits );


#endif

