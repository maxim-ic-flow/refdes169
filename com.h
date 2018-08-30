#ifndef _COM_H_
#define _COM_H_

#include "tdc.h"
#include "flow.h"

void com_init(void);

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

typedef struct _com_tracked_sample_t
{
    max3510x_time_t up;
    max3510x_time_t down;
	max3510x_time_t up_period;
	max3510x_time_t down_period;
}
com_tracked_sample_t;

typedef struct _com_report_t
{
    union
    {
        tdc_result_t        		hits;
        com_tracked_sample_t 		tracked;
		com_interactive_report_t 	interactive;
		
    };
}
com_report_t;

void com_report( report_type_t type, const com_report_t * p_report );

#endif 
