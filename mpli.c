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

// This mechanism provides an interpolated piecewise transformation of the input to the output given a table of in and out values.
// This is useful for systems with non-linear transfer characteristics, like ultrasonic flowbodies.
// Single-point tables are also useful for doing simple linear transforms, like linear unit conversions for example.


mpli_dt mpli_calc( const mpli_t *p_table, mpli_dt in )
{

    mpli_dt out = in;
    uint16_t i;
    uint16_t last = p_table->count-1;
    uint16_t count = p_table->count;
    const mpli_point_t *p = p_table->table;
    if(  in < p[0].x )
        in = p[0].x;
    else if(  in > p[last].x )
        in = p[last].x;
    // this could be improved by using a binary tree algo
    for( i=0;i<last; i++ )
    {
        mpli_dt x1 = p[i].x;
        mpli_dt x2 = p[i+1].x;
        mpli_dt y1 = p[i].y;
        mpli_dt y2 = p[i+1].y;

        if( in <= p[i+1].x )
        {
            out = y1 + (in-x1)*(y2-y1)/(x2-x1);
            break;
        }
    }
    return out;
}


