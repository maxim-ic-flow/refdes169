/*******************************************************************************
 * Copyright (C) 2017 Maxim Integrated Products, Inc., All Rights Reserved.
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

// physical characteristics of the transducers and flowbody
// These default settings may not be appropriate for your transducer configuration

#define DEFAULT_T1_THRESHOLD	127


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
            MAX3510X_BF(SWITCHER1_VS, 27V0),        	// Target VPR voltage
                                                        // Switcher 2
            MAX3510X_BF( SWITCHER2_LT_N, 0V4 )|        	// running current limit V/R14
            MAX3510X_BF( SWITCHER2_LT_S, 0V4 )|        	// startup current limit V/R14
            MAX3510X_REG_SET( SWITCHER2_ST, MAX3510X_REG_SWITCHER2_ST_US( MAX3510X_REG_SWITCHER2_ST_US_MIN ) )| // minimize stabilization time
            MAX3510X_BF( SWITCHER2_LT_50D, TRIMMED )|  	// limit switcher to 50% duty
            MAX3510X_BF( SWITCHER2_PECHO, DISABLED ),   // not using pulse-echo mode
                                                        // AFE 1
            MAX3510X_BF( AFE1_AFE_BP, DISABLED )|      	// not bypassing the AFE
            MAX3510X_BF( AFE1_SD_EN, DISABLED )|       	// output is differential
            MAX3510X_BF( AFE1_AFEOUT, BANDPASS ),       // the ouput of the afe is applied to the CIP/N pins (TP5 and TP6)
                                                        // AFE 2
            MAX3510X_BF( AFE2_4M_BP, DISABLED )|       	// 4MHz clock is from a crystal
            MAX3510X_BF( AFE2_PGA, 21_97DB )|          	// pga gain
            MAX3510X_BF( AFE2_LOWQ, 7_4KHZ )|           // pass band
            MAX3510X_BF( AFE2_BP_BYPASS, DISABLED )     // bandpass bypass is disabled (the bandpass filter is being used)
        },
        {
            MAX3510X_OPCODE_WRITE_REG( MAX3510X_REG_TOF1 ),
            MAX3510X_REG_SET( TOF1_PL, 66 )|            // number of pulses applied to the transducers
            MAX3510X_BF( TOF1_DPL, 400KHZ )|            // pulse launch frequency
            MAX3510X_BF( TOF1_STOP_POL, NEG_EDGE ),		// polarity of the T1 threshold

            MAX3510X_REG_SET( TOF2_STOP, MAX3510X_REG_TOF2_STOP_C( MAX3510X_MAX_HITCOUNT ) )|
            MAX3510X_REG_SET( TOF2_T2WV, 2 )|              // The T2 wave is the 2nd wave after T1
            MAX3510X_BF( TOF2_TOF_CYC, 0US )|              // minimize delay between up and down measurements
            MAX3510X_BF( TOF2_TIMOUT, 512US ),             // timeout
		
            MAX3510X_REG_SET( TOF3_HIT1WV, 13 )|            // hit waves occur immediately after the T2 wave
            MAX3510X_REG_SET( TOF3_HIT2WV, 23 ),
		
            MAX3510X_REG_SET( TOF4_HIT3WV, 33 )|
            MAX3510X_REG_SET( TOF4_HIT4WV, 43 ),
		
            MAX3510X_REG_SET( TOF5_HIT5WV, 53 )|
            MAX3510X_REG_SET( TOF5_HIT6WV, 63 ),
		
            MAX3510X_REG_SET( TOF6_C_OFFSETUPR, 0 )|                   	// hit values correspond to zero crossings of the hit waves
            MAX3510X_REG_SET( TOF6_C_OFFSETUP, DEFAULT_T1_THRESHOLD ),  // T1 threshold
		
            MAX3510X_REG_SET( TOF7_C_OFFSETDNR, 0 )|
            MAX3510X_REG_SET( TOF7_C_OFFSETDN, DEFAULT_T1_THRESHOLD ),
            MAX3510X_REG_SET( EVENT_TIMING_1_TDF, MAX3510X_REG_EVENT_TIMING_1_TDF_S( 1.0 ) ) |	// event mode tof diff samples at 1Hz
            MAX3510X_REG_SET( EVENT_TIMING_1_TMF, MAX3510X_REG_EVENT_TIMING_1_TMF_S( 1.0 ) ) |	// event mode temperature samples at 1Hz
            MAX3510X_REG_SET( EVENT_TIMING_1_TDM, MAX3510X_REG_EVENT_TIMING_1_TDM_C( 1 ) ),		// event mode tof diff collects a single tof diff measurement per sequence
		
            MAX3510X_REG_SET( EVENT_TIMING_2_TMM, MAX3510X_REG_EVENT_TIMING_2_TMM_C( 1 ) )|     // event mode temperature collects a single temperature measurement per sequence
            MAX3510X_BF( EVENT_TIMING_2_CAL_USE, ENABLED )|                                    	// calibration values are applied to measurement results
            MAX3510X_BF( EVENT_TIMING_2_CAL_CFG, DISABLED )|                                   	// calibration is driven by the host cpu.
            MAX3510X_REG_SET( EVENT_TIMING_2_PRECYC, 1 )|
            MAX3510X_BF( EVENT_TIMING_2_PORTCYC, 128US ),										// minimize cycle time

            MAX3510X_REG_SET( TOF_MEASUREMENT_DELAY_DLY, MAX3510X_REG_TOF_MEASUREMENT_DELAY_DLY_US( 200 ) ),   // receiption squelch time
		
            MAX3510X_BF( CALIBRATION_CONTROL_CMP_EN, ENABLED )|     // the output of the comparator is applied to the CMP_OUT pin (TP1)
            MAX3510X_BF( CALIBRATION_CONTROL_CMP_SEL, CMP_EN )|     // enable the CMP_OUT pin
            MAX3510X_BF( CALIBRATION_CONTROL_INT_EN, ENABLED )|     // enable the MAX35104 to issue interrupts via the INT pin
            MAX3510X_BF( CALIBRATION_CONTROL_ET_CONT, DISABLED )|    // one-shot event mode
            MAX3510X_BF( CALIBRATION_CONTROL_CONT_INT, DISABLED )|	// event modes generate an interrupt at the end of the complete sequence
            MAX3510X_BF( CALIBRATION_CONTROL_CLK_S, 1046US )|   
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

const max3510x_registers_t* transducer_config( uint8_t ndx )
{
	// returns the transducer configuration array
	if( ndx >= ARRAY_COUNT(s_config) )
		return NULL;
	return &s_config[ndx];
}

#define FP_OFFSET 0x1E848B26
#define FP_BIAS   5
#define TEMP_COMP_C1    1
#define TEMP_COMP_C2    0


uint32_t transducer_offset_and_scale( const max3510x_fixed_t *p_fixed )
{
    return (max3510x_fixed_to_time( p_fixed ) - FP_OFFSET) << FP_BIAS;
}


static uint32_t aggregate_period( const uint32_t * p_up, const uint32_t * p_down )
{
    uint8_t i;
    uint32_t aggregate_period = 0;
    for( i = 1; i < MAX3510X_MAX_HITCOUNT; i++ )
    {
         aggregate_period += p_up[i] + p_down[i] - p_up[i-1] - p_down[i-1];
    }
    return aggregate_period;
}


uint32_t transducer_tof_from_hits( const max3510x_fixed_t hits[MAX3510X_MAX_HITCOUNT] )
{
    uint8_t i;
    uint32_t sum = 0;
    for(i=0;i<MAX3510X_MAX_HITCOUNT;i++)
    {
       sum += max3510x_fixed_to_time( &hits[i] );
    }
    return (sum - FP_OFFSET ) << FP_BIAS;
}

void transducer_compensated_tof( uint32_t *p_prod, uint32_t *p_up, uint32_t *p_down, const max3510x_fixed_t up[MAX3510X_MAX_HITCOUNT], const max3510x_fixed_t down[MAX3510X_MAX_HITCOUNT] )
{
    uint32_t upp;
    uint32_t downn;

    upp = transducer_tof_from_hits( up );
    downn = transducer_tof_from_hits( down );
    uint32_t ap = aggregate_period( p_up, p_down );
    *p_up = upp * TEMP_COMP_C1 + TEMP_COMP_C2;
    *p_down = downn * TEMP_COMP_C1 + TEMP_COMP_C2;
    *p_prod = ((uint64_t)upp * downn) >> 30;

}

