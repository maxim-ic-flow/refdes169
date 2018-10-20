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
