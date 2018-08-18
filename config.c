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
#include "config.h"
#include "board.h"
#include "flow.h"
#include "transducer.h"

#pragma pack(1)

typedef struct _header_t
{
	uint16_t	size;
	uint16_t	crc;
}
header_t;

#define CONFIG_COUNT	4

typedef struct _data_t
{
	max3510x_registers_t		chip_config;
}
data_t;

typedef struct _config_t
{
	header_t	header;
	union
	{
		data_t		data[CONFIG_COUNT];
		uint32_t    pad[CONFIG_COUNT*(sizeof(data_t)+3)/4];
	};
}
config_t;

#pragma pack()

static config_t s_config;

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
	while( i < CONFIG_COUNT && (p_default = transducer_config(i)) )
	{
		memcpy( &s_config.data[i].chip_config, p_default, sizeof(s_config.data[i].chip_config) );
		i++;
	}
	p_default = transducer_config(0);
	while( i < CONFIG_COUNT )
	{
		memcpy( &s_config.data[i].chip_config, p_default, sizeof(s_config.data[i].chip_config) );
        i++;
	}
	config_save();
}

bool config_set_boot_config( uint8_t ndx )
{
	if( ndx >= CONFIG_COUNT )
		return false;
    if( ndx )
        memcpy( config_get_max3510x_regs(0), config_get_max3510x_regs(ndx), sizeof(s_config.data[ndx].chip_config) );
	config_save();
    return true;
}

max3510x_registers_t * config_load(void)
{
	memset( &s_config, 0, sizeof(s_config) );
	board_flash_read( &s_config, sizeof(s_config) );
	if(  s_config.header.size == sizeof(s_config.pad) )
	{
		uint16_t crc = board_crc( &s_config.pad, sizeof(s_config.pad) );
		if( crc == s_config.header.crc )
		{
			return &s_config.data[0].chip_config;
		}
	}
	// invalid image in flash -- setup defaults.
	config_default();
	return &s_config.data[0].chip_config;
}

max3510x_registers_t* config_get_max3510x_regs( uint8_t ndx )
{
	if( ndx >= CONFIG_COUNT )
		return NULL;
	return &s_config.data[ndx].chip_config;
}

