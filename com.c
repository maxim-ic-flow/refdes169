#include "global.h"
#include "com.h"
#include "board.h"
#include "tdc.h"
#include "config.h"
#include "flow.h"
#include "wave_track.h"

#include <ctype.h>
#include <stdlib.h>

#include "stream_buffer.h"

#include "uart.h"

#include <stdarg.h>
#include <stdio.h>

#define TX_CIRCBUFF_SIZE    4096	// max pending bytes to be output to UART
#define RX_CIRCBUFF_SIZE    128		// max bytes to be stored while command parsing/dispatch is pending
#define TX_MAX_MSG_SIZE     128		// max size of any single message to be output to UART
#define RX_MAX_MSG_SIZE     32		// max size of any single command to be parsed from UART

#define COMMAND_HISTORY_COUNT	4


#define REPORT_MESSAGE_COUNT	4

#define REPORT_TYPE_NATIVE			1
#define REPORT_TYPE_METER			2

typedef enum _com_last_cmd_t
{
	com_last_cmd_tof_diff,
	com_last_cmd_tof_up,
	com_last_cmd_tof_down,
	com_last_cmd_temperature,
	com_last_cmd_cal
}
com_last_cmd_t;
static com_last_cmd_t s_last_cmd;


static QueueSetHandle_t 		s_queue_set;
static StreamBufferHandle_t     s_tx_circbuf;
static StreamBufferHandle_t     s_rx_circbuf;
static StreamBufferHandle_t     s_report_buffer;
static SemaphoreHandle_t        s_tx_semaphore;
static SemaphoreHandle_t        s_rx_semaphore;
static SemaphoreHandle_t        s_report_semaphore;

static char 		s_rx_buf[RX_MAX_MSG_SIZE];
static uint8_t 		s_rx_ndx;
static bool			s_output;

#define CAL_SAMPLE_COUNT	512
#define CAL_SAMPLE_FREQ		128

#define BIT_TIME  ((float_t)(1.0/4000000.0/65536.0))

static uint16_t s_zero_sample_count;
static uint16_t s_flow_sample_count;
static int32_t s_delta_acc;
static flow_dt s_flow_acc;
static uint32_t s_period_acc;

static uint8_t s_prev_freq;

static uint8_t s_report_format;



typedef struct _enum_t
{
	const char *p_tag;
	uint16_t	value;
}
enum_t;

typedef struct _cmd_t
{
	const char *p_cmd;
	const char *p_help;
	bool (*p_set)( const char * );
	void (*p_get)( void );
}
cmd_t;



static void uart_write_cb( void )
{
	size_t size;
	static char buff[128];
	static volatile bool s_primed;

	if( pdTRUE == xPortIsInsideInterrupt() )
	{
		static BaseType_t woken = pdFALSE;
		if( size = xStreamBufferReceiveFromISR( s_tx_circbuf, buff, sizeof(buff), &woken ) )
			board_uart_write( buff, size, uart_write_cb );
		else
			s_primed = false;
		portYIELD_FROM_ISR( woken );
	}
	else
	{
		board_uart_write_lock();
		if( !s_primed )
		{
			if( size = xStreamBufferReceive( s_tx_circbuf, buff, sizeof(buff), 0 ) )
			{
				board_uart_write( buff, size, uart_write_cb );
				s_primed = true;
			}
		}
		board_uart_write_unlock();
	}
}


static void com_printf( const char *p_format, ... )
{
	// this function assumes synchronicity
	static char buff[TX_MAX_MSG_SIZE];
	va_list args;
	va_start( args, p_format );
	vsnprintf( buff, sizeof(buff), p_format, args );
	va_end( args );
	uint16_t len = strlen( buff );
	xStreamBufferSend( s_tx_circbuf, buff, len, portMAX_DELAY );
	uart_write_cb();
}

static const char* skip_space( const char *p )
{
	while( isspace( *p ) ) p++;
	return p;
}

static bool get_enum_value( const char *p_arg, const enum_t *p_enum, uint8_t count, uint16_t *p_result )
{
	uint8_t i;
	for( i = 0; i < count; i++ )
	{
		if( !strncmp( p_arg, p_enum[i].p_tag, strlen( p_enum[i].p_tag ) ) )
		{
			*p_result = p_enum[i].value;
			return true;
		}
	}
	return false;
}

static const char* get_enum_tag( const enum_t *p_enum, uint8_t count, uint16_t value )
{
	uint8_t i;
	for( i = 0; i < count; i++ )
	{
		if( value == p_enum[i].value )
		{
			return p_enum[i].p_tag;
		}
	}
	return NULL;
}

static bool binary( const char *p_arg, uint16_t *p_reg_value )
{
	if( *p_arg == '0' )
	{
		*p_reg_value = 0;
	}
	else if( *p_arg == '1' )
	{
		*p_reg_value = 1;
	}
	else
		return false;
	return true;
}

static const enum_t s_sfreq_enum[] =
{
	{ "100", MAX3510X_REG_SWITCHER1_SFREQ_100KHZ },
	{ "125", MAX3510X_REG_SWITCHER1_SFREQ_125KHZ },
	{ "166", MAX3510X_REG_SWITCHER1_SFREQ_166KHZ },
	{ "200", MAX3510X_REG_SWITCHER1_SFREQ_200KHZ },
};

static bool sfreq_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_sfreq_enum, ARRAY_COUNT( s_sfreq_enum ), &result ) )
	{
		tdc_set_sfreq( result );
		return true;
	}
	return false;
}

static void sfreq_get( void )
{
	uint16_t r = tdc_get_sfreq();
	const char *p = get_enum_tag( s_sfreq_enum, ARRAY_COUNT( s_sfreq_enum ), r );
	com_printf( "%skHz (%d)\r\n", p, r );
}

static const enum_t s_dreq_enum[] =
{
	{ "100", MAX3510X_REG_SWITCHER1_DREQ_100KHZ },
	{ "125", MAX3510X_REG_SWITCHER1_DREQ_125KHZ },
	{ "166", MAX3510X_REG_SWITCHER1_DREQ_166KHZ },
	{ "200", MAX3510X_REG_SWITCHER1_DREQ_200KHZ }
};

static bool dreq_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_dreq_enum, ARRAY_COUNT( s_dreq_enum ), &result ) )
	{
		tdc_set_dreq( result );
		return true;
	}
	return false;
}

static void dreq_get( void )
{
	uint16_t r = tdc_get_dreq();
	const char *p = get_enum_tag( s_sfreq_enum, ARRAY_COUNT( s_sfreq_enum ), r );
	com_printf( "%skHz (%d)\r\n", p, r );
}

static bool hreg_d_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;
	tdc_set_hreg_d( r ? MAX3510X_REG_SWITCHER1_HREG_D_DISABLED : MAX3510X_REG_SWITCHER1_HREG_D_ENABLED );
	return true;
}

static void hreg_d_get( void )
{
	uint16_t r = tdc_get_hreg_d();
	com_printf( "%s\r\n", (r == MAX3510X_REG_SWITCHER1_HREG_D_DISABLED) ? "regulator disabled (1)" : "regulator enabled (0)" );
}

static const float_t s_vpr_target[] = { 27.0f, 25.2f, 23.4f, 21.6f, 19.2f, 17.4f, 15.6f, 13.2f, 11.4f, 9.0f, 7.2f, 5.4f };
static const float_t s_vp_target[] = { 30.6f, 28.8f, 27.0f, 25.2f, 22.8f, 21.0f, 19.2f, 16.8f, 15.0f, 12.6f, 10.8f, 9.0f };
static const uint16_t s_vs_value[] =
{
	MAX3510X_REG_SWITCHER1_VS_27V0,
	MAX3510X_REG_SWITCHER1_VS_25V2,
	MAX3510X_REG_SWITCHER1_VS_23V4,
	MAX3510X_REG_SWITCHER1_VS_21V6,
	MAX3510X_REG_SWITCHER1_VS_19V2,
	MAX3510X_REG_SWITCHER1_VS_17V4,
	MAX3510X_REG_SWITCHER1_VS_15V6,
	MAX3510X_REG_SWITCHER1_VS_13V2,
	MAX3510X_REG_SWITCHER1_VS_11V4,
	MAX3510X_REG_SWITCHER1_VS_9V0,
	MAX3510X_REG_SWITCHER1_VS_7V2,
	MAX3510X_REG_SWITCHER1_VS_5V4_60
};

static bool vs_set( const char *p_arg )
{
	uint8_t i;
	uint16_t reg = 0;
	float_t v = strtof( p_arg, NULL );
	if( v < 5.4f || v > 27.0f  )
		return false;
	for( i = 0; i < ARRAY_COUNT( s_vp_target ); i++ )
	{
		if( v >= s_vpr_target[i] )
		{
			reg = s_vs_value[i];
			com_printf( "vp = %f, vpr = %f\r\n", s_vp_target[i], s_vpr_target[i] );
			break;
		}
	}
	tdc_set_vs( reg );
	return true;
}

static void vs_get( void )
{
	uint8_t i;
	uint16_t r = tdc_get_vs();
	for( i = 0; i < ARRAY_COUNT( s_vs_value ); i++ )
	{
		if( s_vs_value[i] == r )
		{
			com_printf( "%f\r\n", s_vp_target[i] );
			return;
		}
	}
	// not all values are represented by this interface
	com_printf( "%.2fV (%d)\r\n", s_vp_target[i - 1], r );
}

static const enum_t s_lt_n_enum[] =
{
	{ "loop", MAX3510X_REG_SWITCHER2_LT_N_LOOP },
	{ "200", MAX3510X_REG_SWITCHER2_LT_N_0V2 },
	{ "400", MAX3510X_REG_SWITCHER2_LT_N_0V4 },
	{ "800", MAX3510X_REG_SWITCHER2_LT_N_0V8 },
	{ "1600", MAX3510X_REG_SWITCHER2_LT_N_1V6 }
};

static bool lt_n_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_lt_n_enum, ARRAY_COUNT( s_lt_n_enum ), &result ) )
	{
		tdc_set_lt_n( result );
		return true;
	}
	return false;
}

static void lt_n_get( void )
{
	uint16_t r = tdc_get_lt_n();
	const char *p = get_enum_tag( s_lt_n_enum, ARRAY_COUNT( s_lt_n_enum ), r );
	if( r == MAX3510X_REG_SWITCHER2_LT_N_LOOP )
	{
		com_printf( "%s (%d)\r\n", p, r );
	}
	else
	{
		com_printf( "%smV (%d)\r\n", p, r );
	}
}

static const enum_t s_lt_s_enum[] =
{
	{ "none", MAX3510X_REG_SWITCHER2_LT_S_NO_LIMIT },
	{ "200", MAX3510X_REG_SWITCHER2_LT_S_0V2 },
	{ "400", MAX3510X_REG_SWITCHER2_LT_S_0V4 },
	{ "800", MAX3510X_REG_SWITCHER2_LT_S_0V8 },
	{ "1600", MAX3510X_REG_SWITCHER2_LT_S_1V6 }
};


static bool lt_s_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_lt_s_enum, ARRAY_COUNT( s_lt_s_enum ), &result ) )
	{
		tdc_set_lt_s( result );
		return true;
	}
	return false;
}

static void lt_s_get( void )
{
	uint16_t r = tdc_get_lt_s();
	const char *p = get_enum_tag( s_lt_s_enum, ARRAY_COUNT( s_lt_s_enum ), r );
	if( r == MAX3510X_REG_SWITCHER2_LT_N_LOOP )
	{
		com_printf( "%s (%d)\r\n", p, r );
	}
	else
	{
		com_printf( "%smV (%d)\r\n", p, r );
	}
}

static bool st_set( const char *p_arg )
{
	uint16_t us = atoi( p_arg );

	if(  us < MAX3510X_REG_SWITCHER2_ST_US_MIN )
		us = MAX3510X_REG_SWITCHER2_ST_US_MIN;
	else if( us > MAX3510X_REG_SWITCHER2_ST_US_MAX )
		us = MAX3510X_REG_SWITCHER2_ST_US_MAX;

	us += (MAX3510X_REG_SWITCHER2_ST_US_MIN - 1); // round up

	uint16_t r = MAX3510X_REG_SWITCHER2_ST_US( us );
	tdc_set_st( r );

	us = tdc_get_st();
	com_printf( "st = %dus\r\n", us );
	return true;
}

static void st_get( void )
{
	uint16_t r = tdc_get_st();
	uint16_t us = MAX3510X_REG_SWITCHER2_ST( r );
	com_printf( "%dus (%d)\r\n", us, r );
}


static bool lt_50d_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_50d( r ? MAX3510X_REG_SWITCHER2_LT_50D_UNTRIMMED : MAX3510X_REG_SWITCHER2_LT_50D_TRIMMED );
	return true;
}

static void lt_50d_get( void )
{
	uint16_t r = tdc_get_50d();
	com_printf( "%s (%d)\r\n", r ? "untrimmed" : "trimmed", r );
}

static bool pecho_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_pecho( r ? MAX3510X_REG_SWITCHER2_PECHO_ENABLED : MAX3510X_REG_SWITCHER2_PECHO_DISABLED );
	return true;
}

static void pecho_get( void )
{
	uint16_t r = tdc_get_pecho();
	com_printf( "%s (%d)\r\n", r ? "echo mode" : "tof mode", r );
}

static bool afe_bp_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_afe_bp( r ? MAX3510X_REG_AFE1_AFE_BP_DISABLED : MAX3510X_REG_AFE1_AFE_BP_ENABLED );
	return true;
}

static void afe_bp_get( void )
{
	uint16_t r = tdc_get_afe_bp();
	com_printf( "%s (%d)\r\n", r ? "afe bypassed" : "afe enabled", r );
}

static bool sd_en_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_sd( r ? MAX3510X_REG_AFE1_SD_EN_ENABLED : MAX3510X_REG_AFE1_SD_EN_DISABLED );
	return true;
}

static void sd_en_get( void )
{
	uint16_t r = tdc_get_sd();
	com_printf( "%s (%d)\r\n", r ? "single ended drive" : "differential drive", r );
}

static const enum_t s_afeout_enum[] =
{
	{ "disabled", MAX3510X_REG_AFE1_AFEOUT_DISABLED },
	{ "bandpass", MAX3510X_REG_AFE1_AFEOUT_BANDPASS },
	{ "pga", MAX3510X_REG_AFE1_AFEOUT_PGA },
	{ "fga", MAX3510X_REG_AFE1_AFEOUT_FIXED }
};

static bool afeout_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_afeout_enum, ARRAY_COUNT( s_afeout_enum ), &result ) )
	{
		tdc_set_afeout( result );
		return true;
	}
	return false;
}

static void afeout_get( void )
{
	uint16_t r = tdc_get_afeout();
	const char *p = get_enum_tag( s_afeout_enum, ARRAY_COUNT( s_afeout_enum ), r );
	com_printf( "%s (%d)\r\n", p, r );
}

static bool _4m_bp_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_4m_bp( r ? MAX3510X_REG_AFE2_4M_BP_ENABLED : MAX3510X_REG_AFE2_4M_BP_DISABLED );
	return true;
}

static void _4m_bp_get( void )
{
	uint16_t r = tdc_get_4m_bp();
	com_printf( "%s (%d)\r\n", r ? "CMOS clock input" : "oscillator", r );
}

static bool f0_set( const char *p_arg  )
{
	uint16_t r = atoi( p_arg );
	if( r <= MAX3510X_REG_AFE2_F0_MAX )
	{
		tdc_set_f0( r );
		return true;
	}
	return false;
}

static void f0_get( void )
{
	uint16_t r = tdc_get_f0();
	com_printf( "%d\r\n", r );
}

static bool pga_set( const char *p_arg )
{
	float_t gain_db = strtof( p_arg, NULL );
	const float_t min_gain_db = MAX3510X_REG_AFE2_PGA( (float_t)MAX3510X_REG_AFE2_PGA_DB_MIN );
	const float_t max_gain_db = MAX3510X_REG_AFE2_PGA( (float_t)MAX3510X_REG_AFE2_PGA_DB_MAX );
	if( gain_db < min_gain_db || gain_db > max_gain_db )
	{
		return false;
	}
	uint16_t r = (uint16_t)MAX3510X_REG_AFE2_PGA_DB( gain_db );
	tdc_set_pga( r );
	gain_db = MAX3510X_REG_AFE2_PGA( r );
	com_printf( "pga = %.2fdB (%d)\r\n", gain_db, r );
	return true;
}

static void pga_get( void )
{
	uint16_t r = tdc_get_pga();
	com_printf( "%.2fdB(%d)\r\n", MAX3510X_REG_AFE2_PGA( (float_t)r ), r );
}

static const enum_t s_lowq_enum[] =
{
	{ "4.2", MAX3510X_REG_AFE2_LOWQ_4_2KHZ },
	{ "5.3", MAX3510X_REG_AFE2_LOWQ_5_3KHZ },
	{ "7.4", MAX3510X_REG_AFE2_LOWQ_7_4KHZ },
	{ "12", MAX3510X_REG_AFE2_LOWQ_12KHZ }
};

static bool lowq_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_lowq_enum, ARRAY_COUNT( s_lowq_enum ), &result ) )
	{
		tdc_set_lowq( result );
		return true;
	}
	return false;
}

static void lowq_get( void )
{
	uint16_t r = tdc_get_lowq();
	const char *p = get_enum_tag( s_lowq_enum, ARRAY_COUNT( s_lowq_enum ), r );
	com_printf( "%skHz/kHz (%d)\r\n", p, r );
}

static bool bp_bp_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_bp_bp( r ? MAX3510X_REG_AFE2_BP_BYPASS_ENABLED : MAX3510X_REG_AFE2_BP_BYPASS_DISABLED );
	return true;
}

static void bp_bp_get( void )
{
	uint16_t r = tdc_get_bp_bp();
	com_printf( "filter %s (%d)\r\n", r == MAX3510X_REG_AFE2_BP_BYPASS_ENABLED ? "bypassed" : "enabled", r );
}

static bool pl_set( const char *p_arg )
{
	uint16_t r = atoi( p_arg );
	if( r > MAX3510X_REG_TOF1_PL_MAX  )
	{
		return false;
	}
	tdc_set_pl( r );
	return true;
}

static void pl_get( void )
{
	uint16_t r = tdc_get_pl();
	com_printf( "%d pulses\r\n", r );
}

static bool dpl_set( const char *p_arg )
{
	uint16_t r;
	int32_t freq = atoi( p_arg );

	uint8_t i;
	int32_t f;
	int32_t min_delta = 1000000;
	int32_t nearest = 0;

	static const int32_t max_freq = (MAX3510X_REG_TOF1_DPL( BOARD_MAX3510X_CLOCK_FREQ, MAX3510X_REG_TOF1_DPL_MIN ) / 1000);
	static const int32_t min_freq = (MAX3510X_REG_TOF1_DPL( BOARD_MAX3510X_CLOCK_FREQ, MAX3510X_REG_TOF1_DPL_MAX ) / 1000);

	if( freq < min_freq || freq > max_freq )
	{
		return false;
	}

	for( i = MAX3510X_REG_TOF1_DPL_1MHZ; i <= MAX3510X_REG_TOF1_DPL_125KHZ; i++ )
	{
		f = MAX3510X_REG_TOF1_DPL( BOARD_MAX3510X_CLOCK_FREQ / 1000, i );
		if( abs( freq - f ) < min_delta )
		{
			nearest = f;
			min_delta = abs( freq - f );
		}
	}
	r = MAX3510X_REG_TOF1_DPL_HZ( BOARD_MAX3510X_CLOCK_FREQ / 1000, nearest );
	tdc_set_dpl( r );
	com_printf( "dpl = %dkHz (%d)\r\n", nearest, r );
	return true;
}


static void dpl_get( void )
{
	uint16_t r = tdc_get_dpl();
	if( r < MAX3510X_REG_TOF1_DPL_MIN || r > MAX3510X_REG_TOF1_DPL_MAX )
	{
		com_printf( "invalid (%d)\r\n", r );
	}
	else
	{
		com_printf( "%dkHz (%d)\r\n", MAX3510X_REG_TOF1_DPL( (BOARD_MAX3510X_CLOCK_FREQ / 1000), r ), r );
	}
}

static bool stop_pol_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_stop_pol( r ? MAX3510X_REG_TOF1_STOP_POL_NEG_EDGE : MAX3510X_REG_TOF1_STOP_POL_POS_EDGE );
	return true;
}

static void stop_pol_get( void )
{
	uint16_t r = tdc_get_stop_pol();
	com_printf( "%s (%d)\r\n", (r == MAX3510X_REG_TOF1_STOP_POL_NEG_EDGE) ? "negative" : "positive", r );
}

static bool stop_set( const char *p_arg )
{
	uint16_t r, hitcount = atoi( p_arg );
	if( hitcount > MAX3510X_REG_TOF2_STOP_MAX || hitcount < MAX3510X_REG_TOF2_STOP_MIN )
		return false;
	r = MAX3510X_REG_TOF2_STOP_C( hitcount );
	tdc_set_stop( r );
	return true;
}

static void stop_get( void )
{
	uint16_t r = tdc_get_stop();
	uint16_t hitcount = MAX3510X_REG_TOF2_STOP( r );
	com_printf( "hitcount = %d (%d)\r\n", hitcount, r );
}

static bool t2wv_set( const char *p_arg )
{
	uint16_t r = atoi( p_arg );
	if( r > MAX3510X_REG_TOF2_T2WV_MAX || r < MAX3510X_REG_TOF2_T2WV_MIN )
		return false;
	tdc_set_t2wv( r );
	return true;
}

static void t2wv_get( void )
{
	uint16_t r = tdc_get_t2wv();
	com_printf( "wave %d\r\n", r );
}

static const enum_t s_tof_cyc_enum[] =
{
	{ "0", MAX3510X_REG_TOF2_TOF_CYC_0US },
	{ "122", MAX3510X_REG_TOF2_TOF_CYC_122US },
	{ "244", MAX3510X_REG_TOF2_TOF_CYC_244US },
	{ "488", MAX3510X_REG_TOF2_TOF_CYC_488US },
	{ "732", MAX3510X_REG_TOF2_TOF_CYC_732US },
	{ "976", MAX3510X_REG_TOF2_TOF_CYC_976US },
	{ "16650", MAX3510X_REG_TOF2_TOF_CYC_16_65MS },
	{ "19970", MAX3510X_REG_TOF2_TOF_CYC_19_97MS }
};

static bool tof_cyc_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_tof_cyc_enum, ARRAY_COUNT( s_tof_cyc_enum ), &result ) )
	{
		tdc_set_tof_cyc( result );
		return true;
	}
	return false;
}

static void tof_cyc_get( void )
{
	uint16_t r = tdc_get_tof_cyc();
	const char *p = get_enum_tag( s_tof_cyc_enum, ARRAY_COUNT( s_tof_cyc_enum ), r );
	com_printf( "%sms (%d)\r\n", p, r );
}

static const enum_t s_timout_enum[] =
{
	{ "128", MAX3510X_REG_TOF2_TIMOUT_128US },
	{ "256", MAX3510X_REG_TOF2_TIMOUT_256US },
	{ "512", MAX3510X_REG_TOF2_TIMOUT_512US },
	{ "1024", MAX3510X_REG_TOF2_TIMOUT_1024US },
	{ "2048", MAX3510X_REG_TOF2_TIMOUT_2048US },
	{ "4096", MAX3510X_REG_TOF2_TIMOUT_4096US },
	{ "8192", MAX3510X_REG_TOF2_TIMOUT_8192US },
	{ "16384", MAX3510X_REG_TOF2_TIMOUT_16384US }
};

static bool timout_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_timout_enum, ARRAY_COUNT( s_timout_enum ), &result ) )
	{
		tdc_set_timout( result );
		return true;
	}
	return false;
}

static void timout_get( void )
{
	uint16_t r = tdc_get_timeout();
	const char *p = get_enum_tag( s_timout_enum, ARRAY_COUNT( s_timout_enum ), r );
	com_printf( "%sus (%d)\r\n", p, r );
}

#if !defined(MAX35102)

static bool hitwv_set( const char *p_arg )
{
	uint8_t i;
	uint8_t hw[MAX3510X_MAX_HITCOUNT];
	const char *p = p_arg;

	uint16_t tw2v = tdc_get_t2wv();

	for( i = 0; i < MAX3510X_MAX_HITCOUNT; i++ )
	{
		if( !(*p) )
			break;
		if( !isdigit( *p ) )
			return false;
		hw[i] = atoi( p );
		if(  hw[i] < MAX3510X_HITWV_MIN || hw[i] > MAX3510X_HITWV_MAX )
			return false;
		if( !i && hw[i] <= tw2v )
		{
			com_printf( "error:  the first hit wave number must be greater than than the t2 wave number\r\n" );
		}
		if( i && (hw[i] <= hw[i - 1]) )
		{
			com_printf( "error: each hit value must be greater than the previous\r\n" );
			return false;
		}
		if( i == MAX3510X_MAX_HITCOUNT - 1 )
		{
			i++;
			break;
		}
		while( *p && *p != ',' ) p++;
		p = skip_space( p );
		if( !(*p) )
		{
			i++;
			break;
		}
		if( *p != ',' )
			return false;
		p = skip_space( p + 1 );
	}
	// when given a partial list of hit values, assume the remaning hit values are sequential and adjacent.
	for(; i < MAX3510X_MAX_HITCOUNT; i++ )
	{
		hw[i] = hw[i - 1] + 1;
	}
	tdc_set_hitwv( &hw[0] );
	return true;
}

static void hitwv_get( void )
{
	uint8_t hw[MAX3510X_MAX_HITCOUNT];
	tdc_get_hitwv( &hw[0] );
	com_printf( "%d, %d, %d, %d, %d, %d\r\n", hw[0], hw[1], hw[2], hw[3], hw[4], hw[5] );
}

static bool c_offsetupr_set( const char *p_arg )
{
	int8_t r = atoi( p_arg );
	tdc_set_c_offsetupr( r );
	return true;
}

static void c_offsetupr_get( void )
{
	int8_t r = tdc_get_c_offsetupr();
	com_printf( "%d\r\n", r );
}

#endif // #if !defined(MAX35102)

static bool c_offsetup_set( const char *p_arg )
{
	int8_t r = atoi( p_arg );
	flow_set_up_offset( r );
	tdc_set_c_offsetup( r );
	return true;
}

static void c_offsetup_get( void )
{
	int8_t r = tdc_get_c_offsetup();
	com_printf( "%d\r\n", r );
}

#if !defined(MAX35102)

static bool c_offsetdnr_set( const char *p_arg )
{
	int8_t r = atoi( p_arg );
	tdc_set_c_offsetdnr( r );
	return true;
}

static void c_offsetdnr_get( void )
{
	int8_t r = tdc_get_c_offsetdnr();
	com_printf( "%d\r\n", r );
}

#endif

static bool c_offsetdn_set( const char *p_arg )
{
	int8_t r = atoi( p_arg );
	flow_set_down_offset( r );
	tdc_set_c_offsetdn( r );
	return true;
}

static void c_offsetdn_get( void )
{
	int8_t r = tdc_get_c_offsetdn();
	com_printf( "%d\r\n", r );
}

static void tdf_get( void )
{
	uint16_t r = tdc_get_tdf();
#if defined(MAX35103)
	com_printf( "%.2fs (%d)\r\n", (float_t)MAX3510X_REG_EVENT_TIMING_1_TDF( (float_t)r, 0 ), r );
#else
	com_printf( "%.2fs (%d)\r\n", (float_t)MAX3510X_REG_EVENT_TIMING_1_TDF( (float_t)r ), r );
#endif
}

static bool tdf_set( const char *p_arg )
{
	float_t period = strtof( p_arg, NULL );

#if defined(MAX35103)
	uint16_t r = roundf( MAX3510X_REG_EVENT_TIMING_1_TDF_S( period, 0 ) );
#else
	uint16_t r = roundf( MAX3510X_REG_EVENT_TIMING_1_TDF_S( period ) );
#endif
	if( r > MAX3510X_REG_EVENT_TIMING_1_TDF_MAX )
		return false;
	tdc_set_tdf( r );
	tdf_get();
	return true;
}

static void tdm_get( void )
{
	uint16_t r = tdc_get_tdm();
	com_printf( "%d (%d)\r\n", MAX3510X_REG_EVENT_TIMING_1_TDM( r ), r );
}


static bool tdm_set( const char *p_arg )
{
	uint16_t count = atoi( p_arg );
	if(  count < MAX3510X_REG_EVENT_TIMING_1_TDM_MIN || count > MAX3510X_REG_EVENT_TIMING_1_TDM_MAX )
		return false;

	tdc_set_tdm( MAX3510X_REG_EVENT_TIMING_1_TDM_C( count ) );
	tdm_get();
	return true;
}

static bool tmf_set( const char *p_arg )
{
	uint8_t i, min_ndx = 0;
	float_t p = 0.0f, delta;
	float_t delta_min = 1000.0f;
	float_t period = strtof( p_arg, NULL );
#ifdef MAX35103
	if( period < MAX3510X_REG_EVENT_TIMING_1_TMF_S( 0, MAX3510X_REG_EVENT_TIMING_1_TMF_MIN ) ||
		period > MAX3510X_REG_EVENT_TIMING_1_TMF_S( 0, MAX3510X_REG_EVENT_TIMING_1_TMF_MAX ) )
	{
		return false;
	}
#else
	if( period < MAX3510X_REG_EVENT_TIMING_1_TMF_S( MAX3510X_REG_EVENT_TIMING_1_TMF_MIN ) ||
		period > MAX3510X_REG_EVENT_TIMING_1_TMF_S( MAX3510X_REG_EVENT_TIMING_1_TMF_MAX ) )
	{
		return false;
	}
#endif
	for( i = MAX3510X_REG_EVENT_TIMING_1_TMF_MIN; i <= MAX3510X_REG_EVENT_TIMING_1_TMF_MAX; i++ )
	{
		p = i * 1.0f;
		delta = fabsf( period - p );


		if( delta < delta_min )
		{
			delta_min = delta;
			min_ndx = i;
		}
	}
	tdc_set_tmf( min_ndx );

#ifdef MAX35103
	com_printf( "tmf = %.2fs (%d)\r\n", MAX3510X_REG_EVENT_TIMING_1_TMF( 0, min_ndx ), min_ndx );
#else
	com_printf( "tmf = %.2fs (%d)\r\n", MAX3510X_REG_EVENT_TIMING_1_TMF( min_ndx ), min_ndx );
#endif
	return true;
}

static void tmf_get( void )
{
	uint16_t r = tdc_get_tmf();

#ifdef MAX35103
	com_printf( "%ds (%d)\r\n", MAX3510X_REG_EVENT_TIMING_1_TMF( 0, r ), r );
#else
	com_printf( "%ds (%d)\r\n", MAX3510X_REG_EVENT_TIMING_1_TMF( r ), r );
#endif
}


static bool tmm_set( const char *p_arg )
{
	uint16_t count = atoi( p_arg );
	if(  count < MAX3510X_REG_EVENT_TIMING_2_TMM_MIN || count > MAX3510X_REG_EVENT_TIMING_2_TMM_MAX )
		return false;

	tdc_set_tmm( MAX3510X_REG_EVENT_TIMING_2_TMM_C( count ) );
	return true;
}
static void tmm_get( void )
{
	uint16_t r = tdc_get_tmm();
	com_printf( "%d (%d)\r\n", MAX3510X_REG_EVENT_TIMING_2_TMM( r ), r );
}

static bool cal_use_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;
	tdc_set_cal_use( r ? MAX3510X_REG_EVENT_TIMING_2_CAL_USE_ENABLED : MAX3510X_REG_EVENT_TIMING_2_CAL_USE_DISABLED );
	return true;
}

static void cal_use_get( void )
{
	uint16_t r = tdc_get_cal_use();
	com_printf( "%s\r\n", r ? "use calibration data (1)" : "no calibration (0)" );
}

static const enum_t s_cal_cfg_enum[] =
{
	{ "disabled", MAX3510X_REG_EVENT_TIMING_2_CAL_CFG_DISABLED },
	{ "cc", MAX3510X_REG_EVENT_TIMING_2_CAL_CFG_CYCLE_CYCLE },
	{ "cs", MAX3510X_REG_EVENT_TIMING_2_CAL_CFG_CYCLE_SEQ },
	{ "sc", MAX3510X_REG_EVENT_TIMING_2_CAL_CFG_SEQ_CYCLE },
	{ "ss", MAX3510X_REG_EVENT_TIMING_2_CAL_CFG_SEQ_SEQ },
};

static bool cal_cfg_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_cal_cfg_enum, ARRAY_COUNT( s_cal_cfg_enum ), &result ) )
	{
		tdc_set_cal_cfg( result );
		return true;
	}
	return false;
}

static void cal_cfg_get( void )
{
	uint16_t r = tdc_get_cal_cfg();
	const char *p = get_enum_tag( s_cal_cfg_enum, ARRAY_COUNT( s_cal_cfg_enum ), r );
	com_printf( "%s (%d)\r\n", p, r );
}


static bool precyc_set( const char *p_arg )
{
	uint16_t r = atoi( p_arg );
	if( r > MAX3510X_REG_EVENT_TIMING_2_PRECYC_MAX )
		return false;
	tdc_set_precyc( r );
	return true;
}

static void precyc_get( void )
{
	uint16_t r = tdc_get_precyc();
	com_printf( "%d cycles\r\n", r );
}

static const enum_t s_portcyc_enum[] =
{
	{ "128", MAX3510X_REG_TEMPERATURE_PORTCYC_128US },
	{ "256", MAX3510X_REG_TEMPERATURE_PORTCYC_256US },
	{ "284", MAX3510X_REG_TEMPERATURE_PORTCYC_384US },
	{ "512", MAX3510X_REG_TEMPERATURE_PORTCYC_512US }
};

static bool portcyc_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_portcyc_enum, ARRAY_COUNT( s_portcyc_enum ), &result ) )
	{
		tdc_set_portcyc( result );
		return true;
	}
	return false;
}

static void portcyc_get( void )
{
	uint16_t r = tdc_get_portcyc();
	const char *p = get_enum_tag( s_portcyc_enum, ARRAY_COUNT( s_portcyc_enum ), r );
	com_printf( "%sus (%d)\r\n", p, r );
}


static bool dly_set( const char *p_arg )
{
	float_t dly = strtof( p_arg, NULL );
	int16_t r = MAX3510X_REG_TOF_MEASUREMENT_DELAY_DLY_US( dly );

	if( r < MAX3510X_REG_TOF_MEASUREMENT_DELAY_DLY_MIN )
		return false;

	tdc_set_dly( r );

	com_printf( "%.2fus (%d)\r\n", (float_t)MAX3510X_REG_TOF_MEASUREMENT_DELAY_DLY( (float_t)r ), r );

	return true;
}

static void dly_get( void )
{
	int16_t r = tdc_get_dly();
	com_printf( "%.2fus (%d)\r\n", (float_t)MAX3510X_REG_TOF_MEASUREMENT_DELAY_DLY( (float_t)r ), r );
}

static bool cmp_en_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_cmp_en( r ? MAX3510X_REG_CALIBRATION_CONTROL_CMP_EN_ENABLED : MAX3510X_REG_CALIBRATION_CONTROL_CMP_EN_DISABLED );
	return true;
}

static void cmp_en_get( void )
{
	uint16_t r = tdc_get_cmp_en();
	com_printf( "%s\r\n", r ? "enable CMPOUT/UP_DN pin (1)" : "disable CMPOUT/UP_DN pin (0)" );
}

static bool cmp_sel_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_cmp_sel( r ? MAX3510X_REG_CALIBRATION_CONTROL_CMP_SEL_CMP_EN : MAX3510X_REG_CALIBRATION_CONTROL_CMP_SEL_UP_DN );
	return true;
}

static void cmp_sel_get( void )
{
	uint16_t r = tdc_get_cmp_sel();
	com_printf( "%s (%d)\r\n", r == MAX3510X_REG_CALIBRATION_CONTROL_CMP_EN_ENABLED ? "CMPOUT" : "UP_DN", r );
}

static bool et_cont_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_et_cont( r ? MAX3510X_REG_CALIBRATION_CONTROL_ET_CONT_ENABLED : MAX3510X_REG_CALIBRATION_CONTROL_ET_CONT_DISABLED );
	return true;
}

static void et_cont_get( void )
{
	uint16_t r = tdc_get_et_cont();
	com_printf( "%s (%d)\r\n", r == MAX3510X_REG_CALIBRATION_CONTROL_ET_CONT_ENABLED ? "continuous" : "one-shot", r );
}

static bool cont_int_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_cont_int( r ? MAX3510X_REG_CALIBRATION_CONTROL_ET_CONT_ENABLED : MAX3510X_REG_CALIBRATION_CONTROL_CONT_INT_DISABLED );
	return true;
}

static void cont_int_get( void )
{
	uint16_t r = tdc_get_cont_int();
	com_printf( "%s (%d)\r\n", r == MAX3510X_REG_CALIBRATION_CONTROL_ET_CONT_ENABLED ? "continuous" : "one-shot", r );
}

static const enum_t s_clk_s_enum[] =
{
	{ "488", MAX3510X_REG_CALIBRATION_CONTROL_CLK_S_488US },
	{ "1460", MAX3510X_REG_CALIBRATION_CONTROL_CLK_S_1046US },
	{ "2930", MAX3510X_REG_CALIBRATION_CONTROL_CLK_S_2930US },
	{ "3900", MAX3510X_REG_CALIBRATION_CONTROL_CLK_S_3900US },
	{ "5130", MAX3510X_REG_CALIBRATION_CONTROL_CLK_S_5130US },
	{ "continuous", MAX3510X_REG_CALIBRATION_CONTROL_CLK_S_CONTINUOUS }
};

static bool clk_s_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_clk_s_enum, ARRAY_COUNT( s_clk_s_enum ), &result ) )
	{
		tdc_set_clk_s( result );
		return true;
	}
	return false;
}

static void clk_s_get( void )
{
	uint16_t r = tdc_get_clk_s();
	const char *p = get_enum_tag( s_clk_s_enum, ARRAY_COUNT( s_clk_s_enum ), r );
	com_printf( "%s (%d)\r\n", p, r );
}

static bool cal_period_set( const char *p_arg )
{
	float_t period = strtof( p_arg, NULL );
	uint16_t r = (uint16_t)MAX3510X_REG_CALIBRATION_CONTROL_CAL_PERIOD_US( period );
	if( r > MAX3510X_REG_CALIBRATION_CONTROL_CAL_PERIOD_MAX )
	{
		return false;
	}

	tdc_set_cal_period( r );

	period = MAX3510X_REG_CALIBRATION_CONTROL_CAL_PERIOD( r );
	com_printf( "%.2fus (%d)\r\n", (float_t)period, r );
	return true;
}

static void cal_period_get( void )
{
	uint16_t r = tdc_get_cal_period();
	com_printf( "%.2fus (%d)\r\n", (float_t)MAX3510X_REG_CALIBRATION_CONTROL_CAL_PERIOD( r ), r );
}

static bool _32k_bp_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_32k_bp( r ? MAX3510X_REG_RTC_32K_BP_ENABLED : MAX3510X_REG_RTC_32K_BP_DISABLED );
	return true;
}

static void _32k_bp_get( void )
{
	uint16_t r = tdc_get_32k_bp();
	com_printf( "%s (%d)\r\n", r == MAX3510X_REG_RTC_32K_BP_ENABLED ? "CMOS clock input" : "oscillator", r );
}

static bool _32k_en_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;
	tdc_set_32k_en( r ? MAX3510X_REG_RTC_32K_EN_ENABLED : MAX3510X_REG_RTC_32K_EN_DISABLED );
	return true;
}

static void _32k_en_get( void )
{
	uint16_t r = tdc_get_32k_en();
	com_printf( "%s (%d)\r\n", r == MAX3510X_REG_RTC_32K_EN_ENABLED ? "enabled" : "disabled", r );
}

static bool eosc_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_eosc( r ? MAX3510X_REG_RTC_EOSC_ENABLED : MAX3510X_REG_RTC_EOSC_DISABLED );
	return true;
}

static void eosc_get( void )
{
	uint16_t r = tdc_get_eosc();
	com_printf( "%s (%d)\r\n", r == MAX3510X_REG_RTC_EOSC_ENABLED ? "enabled" : "disabled", r );
}

static const enum_t s_am_enum[] =
{
	{ "none", MAX3510X_REG_RTC_AM_NONE },
	{ "minutes", MAX3510X_REG_RTC_AM_MINUTES },
	{ "hours", MAX3510X_REG_RTC_AM_HOURS },
	{ "both", MAX3510X_REG_RTC_AM_HOURS_MINUTES },
};

static bool am_set( const char *p_arg )
{
	uint16_t result;
	if( get_enum_value( p_arg, s_am_enum, ARRAY_COUNT( s_am_enum ), &result ) )
	{
		tdc_set_am( result );
		return true;
	}
	return false;
}

static void am_get( void )
{
	uint16_t r = tdc_get_am();
	const char *p = get_enum_tag( s_am_enum, ARRAY_COUNT( s_am_enum ), r );
	com_printf( "%s (%d)\r\n", p, r );
}

static bool wf_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_wf( r ? MAX3510X_REG_RTC_WF_SET : MAX3510X_REG_RTC_WF_CLEAR );
	return true;
}

static void wf_get( void )
{
	uint16_t r = tdc_get_wf();
	com_printf( "%s (%d)\r\n", r == MAX3510X_REG_RTC_WF_SET ? "set" : "clear", r );
}

static bool wd_en_set( const char *p_arg )
{
	uint16_t r;
	if( !binary( p_arg, &r ) )
		return false;

	tdc_set_wd( r ? MAX3510X_REG_RTC_WD_EN_ENABLED : MAX3510X_REG_RTC_WD_EN_DISABLED );
	return true;
}

static void wd_en_get( void )
{
	uint16_t r = tdc_get_wd();
	com_printf( "%s (%d)\r\n", r == MAX3510X_REG_RTC_WD_EN_ENABLED ? "enabled" : "disabled", r );
}

static char s_command_history[COMMAND_HISTORY_COUNT][RX_MAX_MSG_SIZE];
static uint8_t s_command_ndx;
static uint8_t s_command_read_ndx;

static void record_command( void )
{
	strncpy( &s_command_history[s_command_ndx][0], s_rx_buf, sizeof(s_command_history[0]) - 1 );
	s_command_ndx++;
	if( s_command_ndx >= COMMAND_HISTORY_COUNT )
	{
		s_command_ndx = 0;
	}
	if( s_command_ndx  )
		s_command_read_ndx = s_command_ndx - 1;
	else
		s_command_read_ndx = COMMAND_HISTORY_COUNT - 1;
}

static void retrieve_command( void )
{
	if( s_command_history[s_command_read_ndx][0] )
	{
		strncpy( &s_rx_buf[0], &s_command_history[s_command_read_ndx][0], sizeof(s_rx_buf) - 1 );
		s_rx_ndx = strlen( s_rx_buf );

		com_printf( "\33[2K\r> %s", s_rx_buf );
		if( !s_command_read_ndx )
		{
			s_command_read_ndx = COMMAND_HISTORY_COUNT - 1;
		}
		else
		{
			s_command_read_ndx--;
		}
	}
}


static bool escape( char c )
{
	static uint8_t s_escape;
	// handles cursour up CSI
	bool ret = false;
	if( c == 0x1B )
	{
		s_escape = 1;
		ret = true;
	}
	else if( s_escape == 1 )
	{
		if( c == '[' )
		{
			s_escape = 2;
		}
		ret = true;
	}
	else if(  s_escape == 2 )
	{
		if( c == 'A' )
		{
			retrieve_command();
		}
		s_escape = 0;
		ret = true;
	}
	return ret;
}


static bool tof_up_cmd( const char *p_arg )
{
	tdc_cmd_tof_up();
	s_last_cmd = com_last_cmd_tof_diff;
	return true;
}

static bool tof_down_cmd( const char *p_arg )
{
	tdc_cmd_tof_down();
	s_last_cmd = com_last_cmd_tof_diff;
	return true;
}


static bool tof_diff_cmd( const char *p_arg )
{
	tdc_cmd_tof_diff();
	s_last_cmd = com_last_cmd_tof_diff;
	return true;
}

static bool temp_cmd( const char *p_arg )
{
	tdc_cmd_temperature();
	s_last_cmd = com_last_cmd_temperature;
	return true;
}

static bool reset_cmd( const char *p_arg )
{
	uint8_t i = atoi( p_arg );

	if( config_set_boot_config( i ) )
	{
		com_printf( "reset configuration is %d.\r\n", i );
		board_reset();
	}
	return false;
}

static bool init_cmd( const char *p_arg )
{
	tdc_cmd_initialize();
	return true;
}

static bool bpcal_cmd( const char *p_arg )
{
	tdc_cmd_bpcal();
	return true;
}

static bool halt_cmd( const char *p_arg )
{
	tdc_cmd_halt();
	return true;
}

static bool cal_cmd( const char *p_arg )
{
//	s_last_tdc_cmd = tdc_cmd_cal;
	tdc_cmd_calibrate();
	return true;
}

static bool default_cmd( const char *p_arg )
{
	config_default();
	board_reset();
//	flow_init();
	return true;
}

static void rtrack_get( void )
{
	uint16_t ratio_target = flow_get_ratio_tracking();
	com_printf( "%f\r\n", (float_t)ratio_target / ((float_t)(1 << 16)) );
}

static bool rtrack_set( const char *p_arg )
{
	float_t ratio_target = strtof( p_arg, NULL );
	flow_set_ratio_tracking( ratio_target * ((float_t)(1 << 16)) );
	return true;
}

static void offmin_get( void )
{
	uint8_t minimum_offset = flow_get_minimum_offset();
	com_printf( "%d\r\n", minimum_offset );
}

static bool offmin_set( const char *p_arg )
{
	uint8_t minimum_offset = atoi( p_arg );
	if( minimum_offset > MAX3510X_REG_TOF_C_OFFSETR_MAX )
		return false;
	flow_set_minimum_offset( minimum_offset );
	return true;
}

static bool spi_test_cmd( const char *p_arg )
{
	uint16_t write = ~0;
	uint16_t read;
	uint16_t original = max3510x_read_register( NULL, MAX3510X_REG_TOF_MEASUREMENT_DELAY );
	while( write )
	{
		max3510x_write_register( NULL, MAX3510X_REG_TOF_MEASUREMENT_DELAY, write );
		read = max3510x_read_register( NULL, MAX3510X_REG_TOF_MEASUREMENT_DELAY );
		if(  read != write )
		{
			com_printf( "test failed:  write=%4.4X, read=%4.4X\r\n", write, read );
			return true;
		}
		write--;
	}
	max3510x_write_register( NULL, MAX3510X_REG_TOF_MEASUREMENT_DELAY, original );
	com_printf( "test passed\r\n" );
	return true;
}

static const enum_t s_report_type_enum[] =
{
	{ "tof", COM_REPORT_FORMAT_DETAIL_TOF },
	{ "cal", COM_REPORT_FORMAT_DETAIL_CALIBRATION },
	{ "temp", COM_REPORT_FORMAT_DETAIL_TEMPERATURE },
	{ "tracked", COM_REPORT_FORMAT_TRACKED },
	{ "meter", COM_REPORT_FORMAT_METER }
};

static bool report_cmd( const char *p_arg )
{
	uint16_t r;
	s_report_format = 0;

	while( *p_arg )
	{
		if( !get_enum_value( p_arg, s_report_type_enum, ARRAY_COUNT( s_report_type_enum ), &r ) )
			return false;

		s_report_format |= r;

		while( *p_arg && !isspace( *p_arg ) ) p_arg++;
		p_arg = skip_space( p_arg );
	}
	return true;
}


static bool save_cmd( const char *p_arg )
{
	uint16_t ndx = atoi( p_arg );
	config_t *p_config = config_get( ndx );
	if( p_config )
	{
		max3510x_read_config( NULL, &p_config->chip );
		p_config->algo.calibration_ratio = flow_get_cal_sampling_ratio();
		p_config->algo.sampling_frequency = board_get_sampling_frequency();
		p_config->algo.temperature_ratio = flow_get_temp_sampling_ratio();
		p_config->algo.ratio_tracking = flow_get_ratio_tracking();
		p_config->algo.offset_minimum = flow_get_minimum_offset();

		com_printf( "saved configuration to location %d.\r\n", ndx );
		config_save();
		return true;
	}
	return false;
}


static bool flow_cal_cmd( const char *p_arg )
{
	s_flow_sample_count = CAL_SAMPLE_COUNT;
	s_flow_acc = 0;
	s_prev_freq = flow_get_sampling_frequency();
	flow_set_sampling_frequency( CAL_SAMPLE_FREQ );
	return true;
}


static bool zero_offset_cmd( const char *p_arg )
{
	s_zero_sample_count = CAL_SAMPLE_COUNT;
	s_delta_acc = 0;
	s_period_acc = 0;
	s_prev_freq = flow_get_sampling_frequency();
	flow_set_sampling_frequency( CAL_SAMPLE_FREQ );
	return true;
}

static void tempr_get( void )
{
	com_printf( "%d\r\n", flow_get_temp_sampling_ratio() );
}

static bool tempr_set( const char *p_arg )
{
	uint16_t ratio = atoi( p_arg );
	flow_set_temp_sampling_ratio( ratio );
	return true;
}

static void tofsr_get( void )
{
	com_printf( "%d Hz\r\n", flow_get_sampling_frequency() );
}

static bool calr_set( const char *p_arg )
{
	uint16_t ratio = atoi( p_arg );
	flow_set_cal_sampling_ratio( ratio );
	return true;
}

static void calr_get( void )
{
	com_printf( "%d\r\n", flow_get_cal_sampling_ratio() );
}

static bool squelch_set( const char *p_arg )
{
	uint16_t time_us = atoi( p_arg );
	board_set_squelch_time( time_us );
	return true;
}

static void squelch_get( void )
{
	com_printf( "%dus\r\n", board_get_squelch_time() );
}

static bool tofsr_set( const char *p_arg )
{
	uint8_t freq = atoi( p_arg );
	if( freq > 128 )
	{
		com_printf( "sampling frequency must be 0Hz to 128Hz\r\n" );
		return false;
	}
	flow_set_sampling_frequency( freq );
	return true;
}

static bool help_cmd( const char *p_arg );
static bool dc_cmd( const char *p_arg );

static const cmd_t s_cmd[] =
{
	// command dispatch table

	// TDC register access

	{ "sfreq", "switcher frequency in kHz:  100, 125, 166, or 200", sfreq_set, sfreq_get },
	{ "hreg_d", "high voltage regulator disable:  1=disable regulator, 0=enable regulator", hreg_d_set, hreg_d_get },
	{ "dreq", "doubler freqency in kHz: 100, 125, 166, or 200", dreq_set, dreq_get },
	{ "vs", "voltage select regulator target: 5.4V-27V", vs_set, vs_get },
	{ "lt_n", "limit trim normal operation (mV): loop, 200, 400, 800, 1600", lt_n_set, lt_n_get },
	{ "lt_s", "limit trim startup (mV): none, 200, 400, 800, 1600", lt_s_set, lt_s_get },
	{ "st", "switcher stabilzation time:  64us - 16.4ms", st_set, st_get },
	{ "lt_50d", "limit trim 50% disable:  1=disable trim, 0=enable trim", lt_50d_set, lt_50d_get },
	{ "pecho", "pulse echo: 1=pulse echo mode, 0=time-of-flight mode", pecho_set, pecho_get },
	{ "afe_bp", "AFE bypass: 1=bypass, 0=normal", afe_bp_set, afe_bp_get },
	{ "sd_en", "single ended drive enable:  1=single ended drive, 0=differential drive", sd_en_set, sd_en_get },
	{ "afeout", "AFE output select: disabled, bandpass, pga, or fga", afeout_set, afeout_get },
	{ "4m_bp", "4MHz bypass:  1=using external CMOS clock signal, 0=using crystal oscillator", _4m_bp_set, _4m_bp_get },
	{ "f0", "bandpass center frequency adjustment", f0_set, f0_get },
	{ "pga", "pga gain:  10.00dB - 29.95dB", pga_set, pga_get },
	{ "lowq", "bandpass Q (Hz/Hz):  4.2, 5.3, 7.4, or 12", lowq_set, lowq_get },
	{ "bp_bp", "bandpass filter bypass:  1=filter bypassed, 0=filter active", bp_bp_set, bp_bp_get },
	{ "pl", "pulse launch size:  0-127 pulses", pl_set, pl_get },
	{ "dpl", "pulse launch frequency (kHz): 125 to 1000", dpl_set, dpl_get },
	{ "stop_pol", "comparator stop polarity: 0=positive, 1=negative", stop_pol_set, stop_pol_get },
	{ "stop", "number of stop hits: 1-6", stop_set, stop_get },
	{ "t2wv", "t2 wave select: 2-63", t2wv_set, t2wv_get },
	{ "tof_cyc", "start-to-start time for TOF_DIFF measurements (us): 0, 122, 488, 732, 976, 16650, or 19970", tof_cyc_set, tof_cyc_get },
	{ "timout", "measurement timeout (us): 128, 256, 512, 1024, 2048, 4096, 8192, or 16384", timout_set, timout_get },
	{ "hitwv", "hit wave selection:  comma delimited list of wave numbers", hitwv_set, hitwv_get },
	{ "c_offsetupr", "comparator offset upstream (mV when VCC=3.3V): -137.5 to 136.4", c_offsetupr_set, c_offsetupr_get },
	{ "c_offsetup", "comparator return upstream (mV when VCC=3.3V): 0 to 136.4", c_offsetup_set, c_offsetup_get },
	{ "c_offsetdnr", "comparator offset downstream (mV when VCC=3.3V): -137.5 to 136.4", c_offsetdnr_set, c_offsetdnr_get },
	{ "c_offsetdn", "comparator return downstream (mV when VCC=3.3V): 0 to 136.4", c_offsetdn_set, c_offsetdn_get },
	{ "tdf", "TOF difference measurement period (s):  0.5 to 8.0", tdf_set, tdf_get },
	{ "tdm", "Number of TOF difference measurements to perform: 1 to 32", tdm_set, tdm_get },
	{ "tmf", "temperature measurement period (s):  1 to 64", tmf_set, tmf_get },
	{ "tmm", "Number of temperature measurements to perform: 1 to 32", tmm_set, tmm_get },
	{ "cal_use", "calibration usage:  1=enable, 0=disable", cal_use_set, cal_use_get },
	{ "cal_cfg", "calibration configuration: cc, cs, sc, or ss", cal_cfg_set, cal_cfg_get },
	{ "precyc", "preamble temperature cycle: 0-7", precyc_set, precyc_get },
	{ "portcyc", "port cycle time (us):  128, 256, 384, or 512", portcyc_set, portcyc_get },
	{ "dly", "measurement delay (us):  25 to 16383.75", dly_set, dly_get },
	{ "cmp_en", "comparator or up/down pin enable: 1=enable, 0=disable", cmp_en_set, cmp_en_get },
	{ "cmp_sel", "comparator or up/down select:  1=comparator, 0=up/down", cmp_sel_set, cmp_sel_get },
	{ "et_cont", "event timing continuous operation: 1=continuous, 0=one-shot", et_cont_set, et_cont_get },
	{ "cont_int", "continuous interrupt:  1=continuous, 0=one-shot", cont_int_set, cont_int_get },
	{ "clk_s", "clock settling time (us): 488, 1460, 2930, 3900, 5130, or continuous", clk_s_set, clk_s_get },
	{ "cal_period", "4MHz clock calibration period (us):  30.5 to 488.0", cal_period_set, cal_period_get },
	{ "32k_bp", "32kHz bypass: 1=cmos clock input, 0=crystal input", _32k_bp_set, _32k_bp_get },
	{ "32k_en", "enable 32KOUT pin:  1=enable, 0=disable", _32k_en_set, _32k_en_get },
	{ "eosc", "enable RTC oscillator: 0=enable, 1= disable", eosc_set, eosc_get },
	{ "am", "alarm control:  none, minutes, hours, or both", am_set, am_get },
	{ "wf", "watchdog flag:  0=reset", wf_set, wf_get },
	{ "wd_en", "watchdog enable:  1=enabled, 0=disabled", wd_en_set, wd_en_get },

	// TDC commands

//	{ "event", "start event timing mode: tof, temp, or both", start_event_cmd, NULL },
	{ "tof_up", "TOF_UP command", tof_up_cmd, NULL },
	{ "tof_down", "TOF_DOWN command", tof_down_cmd, NULL },
	{ "tof_diff", "TOF_DIFF command", tof_diff_cmd, NULL },
	{ "tmp", "temperature command", temp_cmd, NULL },
	{ "init", "initialize command", init_cmd, NULL },
	{ "bpcal", "bandpass calibration command", bpcal_cmd, NULL },
	{ "halt", "halt command", halt_cmd, NULL },
	{ "cal", "calibrate command", cal_cmd, NULL },

	// Algorithm settings

	{ "flow", "measure raw flow rate", flow_cal_cmd, NULL },
	{ "zfo", "measure zero flow offset", zero_offset_cmd, NULL },
	{ "dc", "dumps all configuration registers", dc_cmd, NULL },
	{ "rtrack", "track t1/t2 ratio by adjusting comparator offset thresholds", rtrack_set, rtrack_get },
	{ "offmin", "sets the minimum offset used by the wave tracking algorithm", offmin_set, offmin_get },
	{ "tofsr", "host mode sampling frequency (1-128 Hz)", tofsr_set, tofsr_get },
	{ "tempr", "number of TOF measurements per temperature measurement", tempr_set, tempr_get },
	{ "calr", "number of TOF measurements per calibration command", calr_set, calr_get },
	{ "squelch", "signal squelch time (us) zero to disable", squelch_set, squelch_get },

	// platform commands

	{ "reset", "reset command", reset_cmd, NULL },
	{ "save", "save configuration to flash", save_cmd, NULL },
	{ "spi_test", "perform's a write/read verification test on the max3510x", spi_test_cmd, NULL },
	{ "default", "restore configuration defaults", default_cmd, NULL },
	{ "report", "turn on sample reports until a key is pressed:  tof, temp, cal, tracked, meter", report_cmd, NULL },
	{ "help", "you're looking at it", help_cmd, NULL }
};


static void command( uint8_t c )
{
	s_output = false;
	if( escape( c ) )
	{
		if( s_report_format )
		{
			s_report_format = 0;
			com_printf( "\33[2K\r" );
		}
		return;
	}
	if( s_report_format )
	{
		s_report_format = 0;
		com_printf( "\33[2K\r> " );
		if( c == '\r' )
			return;
	}
	com_printf( "%c", c );
	if( c == '\r' )
		com_printf( "\n" );

	if( c != '\r' )
	{
		if( isprint( c ) )
		{
			s_rx_buf[s_rx_ndx++] = c;
		}
		else if( c == 0x7F )
		{
			s_rx_buf[s_rx_ndx] = 0;
			if( s_rx_ndx )
				s_rx_ndx--;
			return;
		}
	}
	else if( c == '\r' || (s_rx_ndx == sizeof(s_rx_buf) - 1) )
	{
		uint8_t i, max_ndx = 0;
		int len, max_len  = 0;
		bool b = false;

		strcpy( &s_rx_buf[0], skip_space( &s_rx_buf[0] ) );

		for( i = 0; i < ARRAY_COUNT( s_cmd ); i++ )
		{
			len = strlen( s_cmd[i].p_cmd );
			if( !strncmp( s_cmd[i].p_cmd, &s_rx_buf[0], len ) )
			{
				if( len > max_len )
				{
					max_len = len;
					max_ndx = i;
				}
			}
		}
		if( max_len )
		{
			len = max_len;
			i = max_ndx;
			const char *p_cmd_type = &s_rx_buf[len];
			if( *p_cmd_type == '?' )
			{
				if( s_cmd[i].p_get )
				{
					s_cmd[i].p_get();
					b = true;
					record_command();
					com_printf( "> " );
				}
				else
				{
					com_printf( "read not supported\r\n> " );
				}
			}
			else
			{
				if( *p_cmd_type == ' ' )
					p_cmd_type = skip_space( p_cmd_type );
				else
				{
					p_cmd_type = skip_space( p_cmd_type );
				}
				if(  *p_cmd_type == '=' )
					p_cmd_type = skip_space( p_cmd_type + 1 );

				if( s_cmd[i].p_set )
				{
					if( !s_cmd[i].p_set( p_cmd_type ) )
						com_printf( "argument error:  %s\r\n> ", s_cmd[i].p_help );
					else
					{
						record_command();
						if( !s_report_format && !s_zero_sample_count && !s_flow_sample_count)
							com_printf( "> " );
					}
					b = true;
				}
				else
				{
					com_printf( "assignment not supported\r\n> " );
				}
			}
		}
		if( b == false )
		{
			if( s_rx_buf[0] )
				com_printf( "unknown command.  type 'help' for a command list.\r\n" );
			com_printf( "> " );
		}
		memset( &s_rx_buf[0], 0, sizeof(s_rx_buf) );
		s_rx_ndx = 0;
		s_output = true;
	}
}


static uint8_t uart_rx_buf[RX_MAX_MSG_SIZE];

static void rx_cb( uart_req_t *p_req, int err )
{
	if( E_NO_ERROR == err )
	{
		BaseType_t woken = pdFALSE;
		if( &p_req->data[1] - uart_rx_buf < sizeof(uart_rx_buf) )
		{
			p_req->data = &p_req->data[1];
		}
		xSemaphoreGiveFromISR( s_rx_semaphore, &woken );
		portYIELD_FROM_ISR( woken );
	}
	else
	{
		while( 1 );
	}
	UART_ReadAsync( BOARD_UART, p_req );
}

static float_t dump_tof( const max3510x_measurement_t *p_dir, uint8_t hitwvs[6], uint16_t hitcount )
{
	const float_t _1us = 1E-6;
	float_t us_sum = 0;
	float_t hit[6];
	float_t period;
	float_t period_sum = 0;
	com_printf( "t2/ideal = %f\r\n", max3510x_ratio_to_float( p_dir->t2_ideal ) );
	com_printf( "t1/t2 = %f\r\n", max3510x_ratio_to_float( p_dir->t1_t2 ) );
	com_printf( "offset/peak = %f\r\n", (float_t)wave_track_linearize_ratio( p_dir->t1_t2 ) / (float_t)(1 << 15) );

	uint8_t i;
	for( i = 0; i < hitcount; i++ )
	{
		hit[i] = max3510x_fixed_to_float( &p_dir->hit[i] );
		if( i )
		{
			period = (hit[i] - hit[i - 1]) / (float_t)(hitwvs[i] - hitwvs[i - 1]);
			period_sum += period;
			com_printf( "hit%d = %.3fus, %.3fkHz\r\n", i + 1, hit[i] / _1us, 1.0f / period / 1000.0f );
		}
		else
		{
			com_printf( "hit%d = %.3fus\r\n", i + 1, hit[i] / _1us );
		}
		us_sum += hit[i];
	}
	float_t hz = (hitcount - 1) * 1.0f / period_sum;
	com_printf( "mean = %.3fus, %.3fkHz\r\n", us_sum / (_1us * hitcount), hz / 1000.0f );
	return hz;
}

static void dump_tof_diff( const tdc_tof_result_t *p_results, uint8_t hitwvs[6], uint16_t hitcount )
{
	const float_t _1ns = 1E-9;
	float_t up_hz = dump_tof( &p_results->tof.up, hitwvs, hitcount );
	com_printf( "\r\n" );
	float_t down_hz = dump_tof( &p_results->tof.down, hitwvs, hitcount );
	com_printf( "diff = %.3fns, %.0fHz\r\n", max3510x_fixed_to_float( &p_results->tof.tof_diff ) / _1ns, (up_hz - down_hz) );
}

static void task_com( void *pv )
{
	static report_type_t report_type;
	static uint8_t rx[RX_MAX_MSG_SIZE];
	static com_report_t report;
	static uart_req_t req =
	{
		.data = uart_rx_buf,
		.len = 1,
		.callback = rx_cb
	};

	QueueSetMemberHandle_t qs;
	com_printf( "\033cMAXREFDES169 v0.1\r\n> " );
	UART_ReadAsync( BOARD_UART, &req );
	while( 1 )
	{
		qs = xQueueSelectFromSet( s_queue_set, portMAX_DELAY );
		if( qs == s_report_semaphore  )
		{
			xSemaphoreTake( s_report_semaphore, 0 );
			while( xStreamBufferReceive( s_report_buffer, &report_type, sizeof(report_type_t), 0 ) < sizeof(report_type_t));

			if( (report_type == report_type_detail) )
			{
				static const tdc_result_t *p = &report.hits;
				xStreamBufferReceive( s_report_buffer, &report, sizeof(report.hits), 0 );
				if( (p->status & MAX3510X_REG_INTERRUPT_STATUS_TOF) && (s_report_format & COM_REPORT_FORMAT_DETAIL_TOF) )
				{
					static const tof_result_t *p_sample = &report.hits.tof.tof;
					com_printf( "r" );
					for( uint8_t i = 0; i < MAX3510X_MAX_HITCOUNT; i++ )
					{
						com_printf( "%4.4X%4.4X", p_sample->up.hit[i].integer, p_sample->up.hit[i].fraction );
					}
					for( uint8_t i = 0; i < MAX3510X_MAX_HITCOUNT; i++ )
					{
						com_printf( "%4.4X%4.4X", p_sample->down.hit[i].integer, p_sample->down.hit[i].fraction );
					}
					com_printf( "%2.2X", p_sample->up.t1_t2 );
					com_printf( "%2.2X", p_sample->down.t1_t2 );
					com_printf( "\r\n" );
				}
				if( (p->status & MAX3510X_REG_INTERRUPT_STATUS_TE) && (s_report_format & COM_REPORT_FORMAT_DETAIL_TEMPERATURE) )
				{
					com_printf( "t" );
					for( uint8_t i = 0; i < 2; i++ ) com_printf( "%4.4X%4.4X", (uint16_t)p->temperature.temperature[i].integer, (uint16_t)p->temperature.temperature[i].fraction );
					com_printf( "\r\n" );
				}
				if( (p->status & MAX3510X_REG_INTERRUPT_STATUS_CAL) && (s_report_format & COM_REPORT_FORMAT_DETAIL_CALIBRATION) )
				{
					com_printf( "c%4.4X%4.4X\r\n", (uint16_t)p->calibration.calibration.integer, (uint16_t)p->calibration.calibration.fraction );
				}
			}
			if( report_type == report_type_native )
			{
				xStreamBufferReceive( s_report_buffer, &report, sizeof(report.interactive), 0 );
				if( report.interactive.cmd_context == tdc_cmd_context_tof_diff )
				{
					max3510x_float_tof_results_t f_results;
					uint8_t hw[MAX3510X_MAX_HITCOUNT];
					max3510x_get_hitwaves( NULL, &hw[0] );
					uint16_t hitcount = MAX3510X_REG_TOF2_STOP( MAX3510X_READ_BITFIELD( NULL, TOF2, STOP ) );
					dump_tof_diff( &report.interactive.tof, hw, hitcount );
					com_printf( "\r\n> " );
				}
			}
			if( report_type == report_type_tracked )
			{
				xStreamBufferReceive( s_report_buffer, &report, sizeof(report.sample), 0 );
				if( s_report_format & COM_REPORT_FORMAT_TRACKED) 
				{
					com_printf( "x%8.8X%8.8X%8.8X%8.8X\r\n", report.sample.up, report.sample.down, report.sample.up_period, report.sample.down_period );
				}
				if( s_zero_sample_count )
				{
					s_delta_acc += report.sample.up - report.sample.down;
					s_period_acc += report.sample.up_period + report.sample.down_period;
					s_zero_sample_count--;
					if( !s_zero_sample_count )
					{
						flow_set_sampling_frequency( s_prev_freq );
						int32_t delta = s_delta_acc / CAL_SAMPLE_COUNT;
						float_t delta_t = (float_t)delta * BIT_TIME * 1000000000.0f;
						int32_t period = s_period_acc / (CAL_SAMPLE_COUNT*2);
						float_t period_t = (float_t)period * BIT_TIME * 1000000.0f;
						com_printf("\33[2Koffset = 0x%8.8X, %.1fns\r\nperiod = 0x%8.8X, %.4fus\r\n> ", delta, delta_t, period, period_t );
					}
				}
			} 
			if( report_type == report_type_meter )
			{
				if(  s_report_format & COM_REPORT_FORMAT_METER )
				{
					xStreamBufferReceive( s_report_buffer, &report, sizeof(report.meter), 0 );
					uint32_t * v = (uint32_t*)&report.meter.volumetric;
					com_printf( "m%f,%f,%f\r\n", report.meter.flow, report.meter.sos, v[1], v[0] );
				}
				if( s_flow_sample_count )
				{
					s_flow_acc += report.meter.flow;
					s_flow_sample_count--;
					if( !s_flow_sample_count )
					{
						flow_set_sampling_frequency( s_prev_freq );
						com_printf("\33[2Kflow = %f\r\n> ", s_flow_acc / CAL_SAMPLE_COUNT );
					}
				}
			}
		}
		else if( qs == s_rx_semaphore && !s_zero_sample_count && !s_flow_sample_count )
		{
			board_uart_disable_interrupt();
			uint8_t rx_count = req.data - uart_rx_buf;
			memcpy( rx, uart_rx_buf, rx_count );
			req.data = uart_rx_buf;
			xSemaphoreTake( s_rx_semaphore, 0 );
			board_uart_enable_interrupt();
			uint8_t ndx = 0;
			while( rx_count-- )
			{
				command( rx[ndx++] );
			}
		}
	};
}

#define MAX_QUEUED_REPORTS    8

void com_init( void )
{
	s_queue_set = xQueueCreateSet( 8 );
	configASSERT( s_queue_set );
	s_tx_semaphore = xSemaphoreCreateBinary();
	configASSERT( s_tx_semaphore );
	s_rx_semaphore = xSemaphoreCreateBinary();
	configASSERT( s_rx_semaphore );
	s_report_semaphore = xSemaphoreCreateCounting( MAX_QUEUED_REPORTS, 0 );
	configASSERT( s_report_semaphore );
	s_report_buffer = xStreamBufferCreate( MAX_QUEUED_REPORTS * (sizeof(com_report_t) + sizeof(report_type_t)), 1 );
	configASSERT( s_report_buffer );
	s_tx_circbuf = xStreamBufferCreate( TX_CIRCBUFF_SIZE, 1 );
	configASSERT( s_tx_circbuf );
	s_rx_circbuf = xStreamBufferCreate( RX_CIRCBUFF_SIZE, 1 );
	configASSERT( s_rx_circbuf );

	xQueueAddToSet( s_report_semaphore, s_queue_set );
	xQueueAddToSet( s_rx_semaphore, s_queue_set );

	xTaskCreate( task_com, "com", TASK_DEFAULT_STACK_SIZE, NULL, TASK_DEFAULT_PRIORITY, NULL );
}

void com_report( report_type_t type, const com_report_t *p_report )
{
	size_t s;
	uint16_t size = 0;
	switch( type )
	{
		case report_type_detail:
		{
			if( s_report_format & (COM_REPORT_FORMAT_DETAIL_TOF | COM_REPORT_FORMAT_DETAIL_TEMPERATURE | COM_REPORT_FORMAT_DETAIL_CALIBRATION) )
				size = sizeof(tdc_result_t);
			break;
		}
		case report_type_meter:
		{
			if( (s_report_format & COM_REPORT_FORMAT_METER) )
				size = sizeof(com_meter_t);
			break;
		}
		case report_type_native:
		{
			size = sizeof(com_interactive_report_t);
			break;
		}
		case report_type_tracked:
		{
			size = sizeof(flowbody_sample_t);
			break;
		}
		default:
			break;
	}
	if( size )
	{
		if( xStreamBufferSpacesAvailable( s_report_buffer ) >= (sizeof(type) + size) )
		{
			xStreamBufferSend( s_report_buffer, &type, sizeof(type), 0 );
			xStreamBufferSend( s_report_buffer, p_report, size, 0 );
			xSemaphoreGive( s_report_semaphore );
		}
	}
}


static bool dc_cmd( const char *p_arg )
{
	uint8_t i;
	uint8_t max_len = 0, len;

	for( i = 0; i < ARRAY_COUNT( s_cmd ); i++ )
	{
		if(  s_cmd[i].p_get )
		{
			len = strlen( s_cmd[i].p_cmd );
			if(  len > max_len )
				max_len = len;
		}
	}
	for( i = 0; i < ARRAY_COUNT( s_cmd ); i++ )
	{
		if(  s_cmd[i].p_get )
		{
			len = strlen( s_cmd[i].p_cmd );
			len = max_len - len;
			com_printf( s_cmd[i].p_cmd );
			while( len-- ) 
				com_printf( " " );
			com_printf( " = " );
			s_cmd[i].p_get();
		}
	}
	return true;
}

static bool help_cmd( const char *p_arg )
{
	uint8_t i;
	uint8_t max_len = 0, len;

	com_printf( "\r\n" );
	for( i = 0; i < ARRAY_COUNT( s_cmd ); i++ )
	{
		len = strlen( s_cmd[i].p_cmd );
		if(  len > max_len )
			max_len = len;
	}
	for( i = 0; i < ARRAY_COUNT( s_cmd ); i++ )
	{
		len = strlen( s_cmd[i].p_cmd );
		com_printf( s_cmd[i].p_cmd );
		len = max_len - len;
		while( len-- ) com_printf( " " );
		com_printf( " - %s\r\n", s_cmd[i].p_help );
	}
	com_printf( "\r\n" );
	return true;
}

