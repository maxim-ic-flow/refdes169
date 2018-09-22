#include "global.h"
#include "mpli.h"

// mpli = mulit-point linearization

// The mpli table defines an array of ranges and linear compensation coefficients.
// Each range is defined by the 'point' member and optionally, the next 'point' member in the array
// The input value 'in' is located within a particular range and the coefficients of that range
// are used to adjust 'in' to provide 'out'

// In the most simple senario, out = in:
// coefficients for this would be coef[0] = 1 and coef[1] = 0 to produce
// out = in * coef[0] + coef[1]
// coefficients are in Q2.30 format, so 1 would be 0x40000000

// With carefully chosen coefficients and ranges, this mechanism provides a piecewise transformation of the input to the output.
// This is necessary for systems with non-linear transfer characteristics, like ultrasonic flowbodies.
// Single-point tables are also useful for doing simple linear transforms, like linear unit conversions for example.


mpli_dt mpli_calc( const mpli_t *p_table, mpli_dt in )
{

	mpli_dt out = in;
	uint8_t i, last = p_table->count-1, count = p_table->count;
	const mpli_point_t *p = p_table->table;
	if(  in < p[0].x )
		in = p[0].x;
	else if(  in > p[last].x )
		in = p[last].x;
	for( i=0;i<last; i++ )
	{
		float_t x1 = p[i].x;
		float_t x2 = p[i+1].x;
		float_t y1 = p[i].y;
		float_t y2 = p[i+1].y;

		if( in < p[i+1].x )
		{
			out = y1 + (in-x1)*(y1-y2)/(x1-x2);
			break;
		}
	}
	return out;
}


