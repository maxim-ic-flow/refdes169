#ifndef _COM_H_
#define _COM_H_

#include "tdc.h"
#include "flow.h"

void com_init(void);

typedef enum _report_type_t
{
    report_type_none,
    report_type_raw,
    report_type_fixed
}
report_type_t;


typedef struct _com_report_t
{
    union
    {
        tdc_result_t        raw;
        flow_accmulation_t fixed;
    };
}
com_report_t;

void com_report( report_type_t type, const com_report_t * p_report );

#endif 
