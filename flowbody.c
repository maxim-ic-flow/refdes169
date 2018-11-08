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

#include "global.h"
#include "board.h"
#include "flowbody.h"
#include "mpli.h"

#define TRANSDUCER_AUDIOWELL_HT0008 0
#define TRANSDUCER_CERAMTEC_09300 1

#define TRANSDUCER TRANSDUCER_AUDIOWELL_HT0008


// This file contains chip configuration and functions specifc to a particular flowbody design.

#define DEFAULT_T1_THRESHOLD	90

static const max3510x_registers_t s_config[1] =
{
    {
        {
            MAX3510X_OPCODE_WRITE_REG( MAX3510X_REG_SWITCHER1 ),
            // Switcher 1
            MAX3510X_BF( SWITCHER1_SFREQ, 100KHZ)|    	// doubler frequency
            MAX3510X_BF(SWITCHER1_HREG_D, ENABLED)|		//
            MAX3510X_BF(SWITCHER1_DREQ, 200KHZ)|     	// switching frequency
            MAX3510X_BF(SWITCHER1_DEFAULT, DEFAULT)|
#if( TRANSDUCER == TRANSDUCER_AUDIOWELL_HT0008 )
            MAX3510X_BF(SWITCHER1_VS, 11V4),        	// Target VPR voltage
#else
            MAX3510X_BF(SWITCHER1_VS, 27V0),        	// Target VPR voltage
#endif
                                                        // Switcher 2
            MAX3510X_BF( SWITCHER2_LT_N, LOOP )|       	// running current limit V/R31
            MAX3510X_BF( SWITCHER2_LT_S, NO_LIMIT )|    // startup current limit V/R31
            MAX3510X_REG_SET( SWITCHER2_ST, MAX3510X_REG_SWITCHER2_ST_US( MAX3510X_REG_SWITCHER2_ST_US_MIN ) )| // minimize stabilization time
            MAX3510X_BF( SWITCHER2_LT_50D, TRIMMED )|  	// limit switcher to 50% duty
            MAX3510X_BF( SWITCHER2_PECHO, DISABLED ),   // not using pulse-echo mode
                                                        // AFE 1
            MAX3510X_BF( AFE1_AFE_BP, ENABLED )|      	// AFE is enabled
            MAX3510X_BF( AFE1_SD_EN, DISABLED )|       	// output is differential
            MAX3510X_BF( AFE1_AFEOUT, BANDPASS ),       // the ouput of the afe is applied to the CIP/N pins (TP5 and TP6)
                                                        // AFE 2
            MAX3510X_BF( AFE2_4M_BP, DISABLED )|       	// 4MHz clock is from a crystal
            MAX3510X_BF( AFE2_PGA, 10_00DB )|          	// pga gain
            MAX3510X_BF( AFE2_LOWQ, 12KHZ )|           // pass band
            MAX3510X_BF( AFE2_BP_BYPASS, DISABLED )     // bandpass bypass is disabled (the bandpass filter is being used)
        },
        {
            MAX3510X_OPCODE_WRITE_REG( MAX3510X_REG_TOF1 ),
#if( TRANSDUCER == TRANSDUCER_AUDIOWELL_HT0008 )
            MAX3510X_REG_SET( TOF1_PL, 4 )|            // number of pulses applied to the transducers
            MAX3510X_BF( TOF1_DPL, 200KHZ )|            // pulse launch frequency
#else
            MAX3510X_REG_SET( TOF1_PL, 8 )|             // number of pulses applied to the transducers
            MAX3510X_BF( TOF1_DPL, 400KHZ )|            // pulse launch frequency
#endif
            MAX3510X_BF( TOF1_STOP_POL, NEG_EDGE ),		// polarity of the T1 threshold

            MAX3510X_REG_SET( TOF2_STOP, MAX3510X_REG_TOF2_STOP_C( MAX3510X_MAX_HITCOUNT ) )|
            MAX3510X_REG_SET( TOF2_T2WV, 2 )|              // The T2 wave is the 2nd wave after T1
            MAX3510X_BF( TOF2_TOF_CYC, 0US )|              // minimize delay between up and down measurements
            MAX3510X_BF( TOF2_TIMOUT, 512US ),             // timeout

            MAX3510X_REG_SET( TOF3_HIT1WV, 8 )|            // hit waves occur immediately after the T2 wave
            MAX3510X_REG_SET( TOF3_HIT2WV, 9 ),

            MAX3510X_REG_SET( TOF4_HIT3WV, 10 )|
            MAX3510X_REG_SET( TOF4_HIT4WV, 11 ),

            MAX3510X_REG_SET( TOF5_HIT5WV, 12 )|
            MAX3510X_REG_SET( TOF5_HIT6WV, 13 ),

            MAX3510X_REG_SET( TOF6_C_OFFSETUPR, 0 )|                   	// hit values correspond to zero crossings of the hit waves
            MAX3510X_REG_SET( TOF6_C_OFFSETUP, DEFAULT_T1_THRESHOLD ),  // T1 threshold

            MAX3510X_REG_SET( TOF7_C_OFFSETDNR, 0 )|
            MAX3510X_REG_SET( TOF7_C_OFFSETDN, DEFAULT_T1_THRESHOLD ),
            MAX3510X_REG_SET( EVENT_TIMING_1_TDF, MAX3510X_REG_EVENT_TIMING_1_TDF_S( 1.0 ) ) |	// event mode tof diff samples at 1Hz
            MAX3510X_REG_SET( EVENT_TIMING_1_TMF, MAX3510X_REG_EVENT_TIMING_1_TMF_S( 1.0 ) ) |	// event mode temperature samples at 1Hz
            MAX3510X_REG_SET( EVENT_TIMING_1_TDM, MAX3510X_REG_EVENT_TIMING_1_TDM_C( 1 ) ),		// event mode tof diff collects a single tof diff measurement per sequence

            MAX3510X_REG_SET( EVENT_TIMING_2_TMM, MAX3510X_REG_EVENT_TIMING_2_TMM_C( 1 ) )|     // event mode temperature collects a single temperature measurement per sequence
            MAX3510X_BF( EVENT_TIMING_2_CAL_USE, ENABLED  )|                                    	// calibration values are applied to measurement results
            MAX3510X_BF( EVENT_TIMING_2_CAL_CFG, DISABLED )|                                   	// calibration is driven by the host cpu.
            MAX3510X_REG_SET( EVENT_TIMING_2_PRECYC, 1 )|
            MAX3510X_BF( EVENT_TIMING_2_PORTCYC, 256US ),

            MAX3510X_REG_SET( TOF_MEASUREMENT_DELAY_DLY, MAX3510X_REG_TOF_MEASUREMENT_DELAY_DLY_US( 220 ) ),   // receiption squelch time

            MAX3510X_BF( CALIBRATION_CONTROL_CMP_EN, ENABLED )|     // the output of the comparator is applied to the CMP_OUT pin (TP1)
            MAX3510X_BF( CALIBRATION_CONTROL_CMP_SEL, CMP_EN )|     // enable the CMP_OUT pin
            MAX3510X_BF( CALIBRATION_CONTROL_INT_EN, ENABLED )|     // enable the MAX35104 to issue interrupts via the INT pin
            MAX3510X_BF( CALIBRATION_CONTROL_ET_CONT, DISABLED )|    // one-shot event mode
            MAX3510X_BF( CALIBRATION_CONTROL_CONT_INT, DISABLED )|	// event modes generate an interrupt at the end of the complete sequence
            MAX3510X_BF( CALIBRATION_CONTROL_CLK_S, CONTINUOUS )|
            MAX3510X_BF( CALIBRATION_CONTROL_CAL_PERIOD, MAX ),		// calibration time MAX=488us

            MAX3510X_BF( RTC_32K_BP, DISABLED )|       // using a 32.768kHz crystal
            MAX3510X_BF( RTC_32K_EN, DISABLED )|       // disable 32KOUT
            MAX3510X_BF( RTC_EOSC, DISABLED )|         // not using the RTC, so disable it's clock.
            MAX3510X_BF( RTC_AM, NONE )|               // not using the RTC
            MAX3510X_BF( RTC_WF, CLEAR )|
            MAX3510X_BF( RTC_WD_EN, DISABLED )          // firmware does not use the MAX35104's watchdog
        }
    }
};

static const config_algo_t s_algo =
{
    // transducer-specific defaults for the tracking algorithm and other firmware-specific values
	.offset_minimum = 20,
	.temperature_ratio = 20,
};

bool flowbody_config( config_t *p_config, uint8_t ndx )
{
	if( ndx >= ARRAY_COUNT(s_config) )
		return false;
	memcpy( &p_config->chip, &s_config[ndx], sizeof(p_config->chip));
	memcpy( &p_config->algo, &s_algo, sizeof(p_config->algo) );
	return true;
}

#define TOF_MIN	0x40A1EF2
#define TOF_MAX 0x51C0A56

// TOF_MIN and TOF_MAX describe the range of time-of-flight values expected from the TDC across the operating range
// of the flowbody.  This allows for offsetting to minimize precision loss incured by converting to
// TDC time integers to single-point floats.

// For this flowbody, TOF values require 27 bits (-10C to +40C, dry air).  Offsetting reduce this to 25 bits.
// 2 bits of precision are lost during conversion to sp-float instead of 4 bits.

// TOF_MIN can be set to zero when working with an unknown flowbody.

void flowbody_transducer_compensate( const flowbody_sample_t *p_sample, uint32_t * p_up, uint32_t * p_down )
{
    // This firmware does not provide transducer-specific temperature compensation.

	// As a result, flow data from this firmware is dependant on temperature.  At a single temperature,
	// it is common to see a non-zero flow, which is referred to as "zero-flow offset".  It can be positive or negative
	// and dependant on the specific transducer pair installed into the flowbody, their temperature, temperature differential
	// within a transducer, and between the transducers.

	// If you intend to productize this firmware, this routine represents a code stub in which
	// you can impliment your transducer temperature compensation algorithm.  'p_sample' contains measurements
	// of directional transducer oscillation period which strongly corrolates with transducer temperature-dependant delays.
	// However, other inputs, like an actual temperature measurement, can also be used to drive the compensation algorithm.
	// See Maxim Application Note 6631 for more information.

	static const mpli_point_t table[] =
	{
        { TOF_MIN, 0 }, { TOF_MAX, 0 }
	};
	static const mpli_t mpli =
	{
		.table = table,
		.count = ARRAY_COUNT(table),
	};

	// provided as an example if you want to use mpli.
	int32_t offset_adjust = (int32_t)mpli_calc( &mpli, (mpli_dt)(p_sample->up_period + p_sample->down_period ) );
    *p_up = p_sample->up - offset_adjust;
    *p_down = p_sample->down + offset_adjust;
}

void flowbody_flow_sos( max3510x_time_t up, max3510x_time_t down, flow_dt *p_flow, flow_dt *p_sos )
{
    // Convert time-of-flight data to flow and speed-of-sound using the sum over product and difference over product methods
    static const mpli_point_t table[] =
    {
        // This table contian
        { -1E-6, -1E-6 }, { 1E-6, 1E-6 }
    };
    static const mpli_t mpli =
    {
        .table = table,
        .count = ARRAY_COUNT(table),
    };

	const uint32_t offset = TOF_MIN;

	uint32_t upo = up - offset;
	uint32_t downo = down - offset;

	float_t dtf = (float_t)(up - down);
	float_t upf = (float_t)(up - offset);
	float_t downf = (float_t)(down - offset);
	float_t pf = upf * downf;


	float_t flow = dtf / pf;                // This quantity is proportional to gas flow through the flowbody
                                            // and is independant of the speed-of-sound.  Typical magnitude is ~1E-10

    // The intent here is to linearlize the flow response of the flowbody as well as correct for model errors.
    // The interpolation table is specific to the physical aspects of the flowbody.
    mpli_dt linearized_flow = mpli_calc( &mpli, flow );

	*p_flow = (flow_dt)linearized_flow;

	if( p_sos )
	{
        float_t sf = (float_t)(up + down - offset<<1 );
        float_t sos =  sf / pf;
        *p_sos = (flow_dt)sos;      // This quantity is proportional to the speed-of-sound within the flowbody
                                    // and is independant of flow velocity.
	}
}

flow_dt flowbody_volumetric( flow_dt integrated_flow, uint32_t sample_count )
{
	static const flow_dt velocity_to_volumetric = 1.0f;  // Q2.30

	// given linearized flow, this function returns a volumetric value with the assumption
	// that volumetric flow is scaled version of linear flow.

	flow_dt v = integrated_flow / sample_count;
	return v;
}


float_t flowbody_hmi_flow( flow_dt flow )
{
    return 0;
}
float_t flowbody_hmi_sos( flow_dt sos )
{
    return 0;
}
float_t flowbody_hmi_volumetric( int64_t volumetric )
{
    return 0;
}

