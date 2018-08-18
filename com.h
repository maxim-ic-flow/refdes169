
#include "tdc.h"

void com_init(void);

typedef struct _com_report_t
{
	tdc_result_t    sample;
}
com_report_t;


void com_report( const com_report_t * p_report );
