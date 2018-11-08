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

#ifndef _COM_H_
#define _COM_H_

#include "tdc.h"
#include "flow.h"
#include "flowbody.h"

void com_init( void );

typedef enum _report_type_t
{
    report_type_none,
    report_type_meter,
    report_type_detail,
    report_type_tracked,
    report_type_native
}
report_type_t;

#define COM_REPORT_FORMAT_DETAIL_TOF			1
#define COM_REPORT_FORMAT_DETAIL_TEMPERATURE	2
#define COM_REPORT_FORMAT_DETAIL_CALIBRATION	4

#define COM_REPORT_FORMAT_METER			8
#define COM_REPORT_FORMAT_NATIVE		16
#define COM_REPORT_FORMAT_TRACKED		32



typedef struct _com_interactive_report_t
{
    tdc_tof_result_t	tof;
    tdc_cmd_context_t	cmd_context;
}
com_interactive_report_t;


typedef struct _com_meter_t
{
    flow_dt     flow;
    flow_dt	sos;
    flow_dt     volumetric;
}
com_meter_t;

typedef struct _com_report_t
{
    union
    {
        tdc_result_t        		hits;
        flowbody_sample_t 		sample;
        com_interactive_report_t 	interactive;
        com_meter_t			meter;
    };
}
com_report_t;

void com_report( report_type_t type, const com_report_t * p_report );

#endif
