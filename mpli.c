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
#include "mpli.h"

// mpli = mulit-point linearization

// The mpli table defines an array of ranges and linear compensation coefficients.
// Each range is defined by the 'point' member and optionally, the next 'point' member in the array
// The input value 'in' is located within a particular range and the coefficients of that range
// are used to adjust 'in' to provide 'out'

// In the most simple senario, out = in:
// coefficients for this would be coef[0] = 1 and coef[1] = 0 to produce
// out = in * coef[0] + coef[1]

// This mechanism provides a piecewise transformation of the input to the output.
// This is useful for systems with non-linear transfer characteristics, like ultrasonic flowbodies.
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


