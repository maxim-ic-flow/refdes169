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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "global.h"
#include "config.h"
#include "board.h"
#include "flow.h"
#include "flowbody.h"

#pragma pack(1)

typedef struct _header_t
{
	uint16_t	size;
	uint16_t	crc;
}
header_t;

#define CONFIG_COUNT	4

typedef struct _flash_config_t
{
	header_t	header;
	union
	{
		config_t	data[CONFIG_COUNT];
		uint32_t    pad[CONFIG_COUNT*(sizeof(config_t)+3)/4];
	};
}
flash_config_t;

#pragma pack()

static flash_config_t s_config;

void config_save( void )
{
	s_config.header.size = sizeof(s_config.pad);
	uint16_t crc = board_crc( &s_config.pad, sizeof(s_config.pad) );
	s_config.header.crc = crc;
	board_flash_write( &s_config, sizeof(s_config) );
}

void config_default( void )
{
	uint8_t i= 0;
    memset( &s_config, 0, sizeof(s_config) );
	const max3510x_registers_t *p_default;
	while( i < CONFIG_COUNT && flowbody_config( &s_config.data[i], i ) )
		i++;
	while( i < CONFIG_COUNT )
	{
		flowbody_config( &s_config.data[i], 0 ) ;
        i++;
	}
	config_save();
}

bool config_set_boot_config( uint8_t ndx )
{
	if( ndx >= CONFIG_COUNT )
		return false;
    if( ndx )
        memcpy( config_get(0), config_get(ndx), sizeof(s_config.data) );
	config_save();
    return true;
}

config_t * config_get( uint8_t ndx )
{
	if( ndx >= CONFIG_COUNT )
		return NULL;
	return &s_config.data[ndx];
}

const config_t * config_load(void)
{
	memset( &s_config, 0, sizeof(s_config) );
	board_flash_read( &s_config, sizeof(s_config) );
	if(  s_config.header.size == sizeof(s_config.pad) )
	{
		uint16_t crc = board_crc( &s_config.pad, sizeof(s_config.pad) );
		if( crc == s_config.header.crc )
		{
			return config_get(0);
		}
	}
	// invalid image in flash -- setup defaults.
	config_default();
	return config_get(0);
}


#endif
