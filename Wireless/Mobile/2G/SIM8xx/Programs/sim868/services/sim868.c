/*
 * sim868.c
 *
 * Created: 20/01/2019 16:20:49
 *  Author: Danil Murashkin
 */ 

#include "../config/ide_config.h"
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "sim868.h"
#include "sim868_data.h"
#include "../config/sim868_config.h"
#include "../drivers/interrupts.h"
#include "../drivers/gpio.h"
#include "../drivers/usart.h"
#include "../utilities/functions.h"




unsigned int  sim868_buffer_pointer;
unsigned char sim868_buffer_flag;
unsigned int  sim868_buffer_write_len;

char sim868_responce_buf[ SIM868_BUFFER_SIZE ];
unsigned int sim868_responce_buf_len;
unsigned int sim868_responce_buf_len_max = SIM868_BUFFER_SIZE;
const char* sim868_responce;
unsigned char sim868_responce_flag;
unsigned int  sim868_responce_pointer;
unsigned int  sim868_responce_write_pointer_begin;
unsigned int  sim868_responce_write_pointer_end;
unsigned int  sim868_responce_len;
unsigned int  sim868_responce_line;
unsigned char sim868_responce_write_flag;

char sim868_readed_char;
unsigned char sim868_buffer_stop_char;



void sim868_power_en(void);
void sim868_power_dis(void);

void sim868_get_char(char *data);
void sim868_print_char(char data);

unsigned char sim868_http_read(unsigned int responce_len, unsigned int time_data_wait);
unsigned char sim868_write_buff(unsigned int write_len, unsigned int timeout);
unsigned char sim868_http_action(unsigned int *responce_len);
unsigned char sim868_http_init(const char* host, const char* path, const char* params);
unsigned char sim868_http_close(void);
unsigned char sim868_gprs_init(void);
unsigned char sim868_gprs_init_base(void);
unsigned char sim868_gprs_close(void);
unsigned char sim868_gsm_check(void);

unsigned char sim868_command_responce(const char* command, const char* responce);
unsigned char sim868_command_responce_http(const char* command, const char* responce);
unsigned char sim868_command_responce_http_para(const char* command, const char* responce);
void sim868_wait_responce_begin (const char* responce);
unsigned char sim868_wait_responce_end (unsigned int timeout, unsigned char lineout);

void sim868_delay(unsigned int delay_time);
void sim868_buffer_print(char *buffer, unsigned int start_point, unsigned int end_point);

void reset(void);





void reset(void)
{
	//sim868_power_dis();
	//sim868_power_en();
	asm("jmp 0x0000");
}

void sim868_example_request(void)
{	
	if( sim868_command_responce(sim868_CmdCReg, sim868_CmdCRegResp) == GOOD_CODE  )
	//if( sim868_command_responce(sim868_command__at, sim868_data__error) == GOOD_CODE )
	{
		sim868_command_responce(sim868_command__at, sim868_data__ok);
	}
	else
	{
		sim868_command_responce(sim868_data__at_plus, sim868_data__error);
	}
}



unsigned char sim868_get_location( char* latitude, unsigned char* latitude_len, char* longtitude, unsigned char* longtitude_len )
{
	sim868_command_responce( sim868_command__gnss_get_info, sim868_data__gnss_get_info );
	sim868_write_buff(50, 300);
	//sim868_buffer_print( sim868_buffer, 0, 50 );
	
	
	*latitude_len = 1;
	*longtitude_len = 1;
	latitude[0] = '0';
	longtitude[0] = '0';

	if( sim868_buffer[0] == '1' ) //have fix position
	{
		unsigned char data_pointer = 0;		
		unsigned char _latitude_len = 0;
		unsigned char _longtitude_len = 0;
		
		for( unsigned char i = 21; i < 50; i++ )
		{					
			unsigned char ch;				
			ch = sim868_buffer[i];	
			
			if( ch == ',' )
			{
				data_pointer++;
				continue;
			}
			
			switch( data_pointer )
			{
				case 0:					
					latitude[ _latitude_len++ ] = ch;
				break;
				
				case 1:
					longtitude[ _longtitude_len++ ] = ch;
				break;
				
				default:
					*latitude_len = _latitude_len;
					*longtitude_len = _longtitude_len;
					
					return GOOD_CODE;
				break;
			}				
		}//for( unsigned char i = 20; i < 50; i++ )
		
	}//if( sim868_buffer[0] == '1' )	
	
	return GOOD_CODE;
}



unsigned char sim868_request_get_send( const char* host, const char* path, const char* params, unsigned int *responce_len )
{
	//sim868_print_newstr(); sim868_print_chararr(host); sim868_print_chararr(path); sim868_print_chararr(params); sim868_print_newstr();
	
	unsigned int resp_len=0;
	*responce_len = 0;
	
	unsigned char recturn_code = GOOD_CODE;
	
	//_delay_ms(1000);
	if( (recturn_code == GOOD_CODE) && ( sim868_gsm_check() )  ) recturn_code = ERROR_CODE;
	if( (recturn_code == GOOD_CODE) && ( sim868_gprs_init() )  ) recturn_code = ERROR_CODE;	
	if( (recturn_code == GOOD_CODE) && ( sim868_http_init(host, path, params) )  ) recturn_code = ERROR_CODE;	
	if( (recturn_code == GOOD_CODE) && ( sim868_http_action(&resp_len) ) ) recturn_code = ERROR_CODE;
	if( (recturn_code == GOOD_CODE) && ( sim868_http_read(resp_len + SIM868_HTTPREAD_HEADER_LEN + 5, 700) )  ) recturn_code = ERROR_CODE;	
	
	if( recturn_code == GOOD_CODE )
	{
		*responce_len = resp_len;
		for( unsigned int i=0; i<resp_len; i++ )
		{
			sim868_buffer[i] = sim868_responce_buf[ sim868_responce_write_pointer_begin+5+i ];
		}
		//sim868_buffer_print( sim868_buffer, 0, resp_len );
	}
	
	sim868_request_get_end();
	
	return recturn_code;
}

unsigned char sim868_request_get_end(void)
{
	sim868_http_close();
	sim868_gprs_close();
	
	return GOOD_CODE;
}

unsigned int sim868_buffer_to_uint( char *buffer_data, unsigned int start_pointer, unsigned int end_pointer )
{
	if( end_pointer <= start_pointer ) return 0;
	if( (end_pointer-start_pointer) > UINT_LEN ) return 0;
	
	unsigned int digit = 1;
	unsigned int result = 0;	
	unsigned int i = end_pointer;
	
	while( (i--) > start_pointer )	
	{
		if( (buffer_data[i] > 47) && (buffer_data[i] < 58) ) 
		{
			result += ( buffer_data[i] - 48 ) * digit;
			digit *= 10;
		}
	}
	
	return result;
}





unsigned char sim868_http_read(unsigned int responce_len, unsigned int time_data_wait)
{
	sim868_command_responce_http(sim868_CmdHttpRead, sim868_RespHttpRead);	
	//if( sim868_write_buff(responce_len, time_data_wait) ) return ERROR_CODE;
	sim868_write_buff(responce_len, time_data_wait);
	
	return GOOD_CODE;
}

unsigned char sim868_write_buff(unsigned int write_len, unsigned int timeout)
{
	unsigned int tick = 0;	
	while( tick++ < timeout )
	{
		if( write_len <= sim868_responce_buf_len )
		{
			sim868_responce_write_pointer_end = sim868_responce_buf_len;
			break;
		}
		
		_delay_ms( SIM868_TIMEOUT_TICK );
	}
	
	return GOOD_CODE;
}

unsigned char sim868_http_action(unsigned int *responce_len)
{
	*responce_len = 0;
	
	sim868_wait_responce_begin( sim868_RespHttpAct200 );
	sim868_print_progmem( sim868_TextHttp );
	sim868_print_progmem( sim868_CmdHttpAct );
	
	if( sim868_wait_responce_end(6000, 4) ) return ERROR_CODE;
	sim868_responce_write_pointer_end = sim868_responce_buf_len;	
	
	*responce_len = sim868_buffer_to_uint( sim868_responce_buf, sim868_responce_write_pointer_begin+1, sim868_responce_write_pointer_end-1 );
	
	return GOOD_CODE;
}

unsigned char sim868_http_init(const char* host, const char* path, const char* params)
{
	if( sim868_command_responce_http( sim868_CmdHttpInit, sim868_data__ok ) )
	{
		sim868_command_responce_http( sim868_CmdHttpTerm, sim868_data__ok );
		if( sim868_command_responce_http( sim868_CmdHttpInit, sim868_data__ok ) )	return ERROR_CODE;
	}
	if( sim868_command_responce_http_para( sim868_CmdHttpParaCid1, sim868_data__ok ) ) return ERROR_CODE;
	
	sim868_wait_responce_begin( sim868_data__ok );
	sim868_print_progmem( sim868_TextHttp );
	sim868_print_progmem( sim868_TextPara );
	sim868_print_progmem( sim868_CmdHttpParaUrl );
	sim868_print_chararr( host );
	sim868_print_chararr( path );
	sim868_print_chararr( params );
	sim868_print_progmem( sim868_CmdHttpParaUrlEnd );
	
	if (sim868_wait_responce_end(600, 2)) return ERROR_CODE;
	
	if (sim868_command_responce_http_para(sim868_CmdHttpParaContApl, sim868_data__ok)) return ERROR_CODE;
	
	return GOOD_CODE;
}

unsigned char sim868_http_close(void)
{
	if( sim868_command_responce_http( sim868_CmdHttpTerm, sim868_data__ok) == GOOD_CODE  ) return GOOD_CODE;
	
	if( (sim868_command_responce_http( sim868_CmdHttpInit, sim868_data__ok) != GOOD_CODE) && 
		(sim868_command_responce_http( sim868_CmdHttpTerm, sim868_data__ok) != GOOD_CODE) ) return ERROR_CODE;
	
	return GOOD_CODE;
}

unsigned char sim868_gprs_init(void)
{
	if( sim868_gprs_init_base() )
	{
		if( (sim868_command_responce(sim868_CmdCReg, sim868_CmdCRegResp) == GOOD_CODE) ||
			(sim868_command_responce(sim868_CmdCReg, sim868_CmdCRegResp) == GOOD_CODE) ||
			(sim868_command_responce(sim868_CmdCReg, sim868_CmdCRegResp) == GOOD_CODE) )
		{
			return GOOD_CODE;
		}
		
		reset();
		
		if( (sim868_command_responce(sim868_CmdCReg, sim868_CmdCRegResp) != GOOD_CODE) &&
			(sim868_command_responce(sim868_CmdCReg, sim868_CmdCRegResp) != GOOD_CODE) &&
			(sim868_command_responce(sim868_CmdCReg, sim868_CmdCRegResp) != GOOD_CODE) )
		{
			return ERROR_CODE;
		}
		
		return sim868_gprs_init_base();
	}
	
	return GOOD_CODE;
}

unsigned char sim868_gprs_init_base(void)
{
	if( sim868_command_responce(sim868_CmdSapbr31Gprs, sim868_data__ok) &&
		sim868_command_responce(sim868_CmdSapbr31Gprs, sim868_data__ok) ) return ERROR_CODE;
	
	_delay_ms(200);
	if( sim868_command_responce(sim868_CmdSapbrGprs11, sim868_data__ok) == GOOD_CODE ) 
	{
		_delay_ms(200);
		return GOOD_CODE;
	}
	_delay_ms(200);
	sim868_command_responce(sim868_CmdSapbrGprs01, sim868_data__ok);
	_delay_ms(200);
	if( sim868_command_responce(sim868_CmdSapbrGprs11, sim868_data__ok) != GOOD_CODE ) 
	{
		_delay_ms(200);
		return ERROR_CODE;
	}
	
	return GOOD_CODE;
}

unsigned char sim868_gprs_close(void)
{
	if( sim868_command_responce(sim868_CmdSapbrGprs01, sim868_data__ok) == GOOD_CODE ) 
	{
		_delay_ms(200);
		return GOOD_CODE;
	}
	_delay_ms(200);
	sim868_command_responce(sim868_CmdSapbrGprs11, sim868_data__ok);
	_delay_ms(200);
	if( sim868_command_responce(sim868_CmdSapbrGprs01, sim868_data__ok) == GOOD_CODE ) 
	{
		_delay_ms(200);
		return GOOD_CODE;
	}
	
	return ERROR_CODE;
}

unsigned char sim868_gsm_check(void)
{
	if( (sim868_command_responce(sim868_CmdCReg, sim868_CmdCRegResp) == GOOD_CODE) ||
		(sim868_command_responce(sim868_CmdCReg, sim868_CmdCRegResp) == GOOD_CODE) ||
		(sim868_command_responce(sim868_CmdCReg, sim868_CmdCRegResp) == GOOD_CODE) ) 
	{
		return GOOD_CODE;
	}
	
	reset();
		
	return sim868_command_responce( sim868_CmdCReg, sim868_CmdCRegResp );
	
	return ERROR_CODE;
}



unsigned char sim868_command_responce(const char* command, const char* responce)
{
	sim868_wait_responce_begin( responce );	
	
	sim868_print_progmem( command );
	
	return sim868_wait_responce_end(600, 2);
}

unsigned char sim868_command_responce_http(const char* command, const char* responce)
{
	sim868_wait_responce_begin( responce );
	
	sim868_print_progmem( sim868_TextHttp );
	sim868_print_progmem( command );
	
	return sim868_wait_responce_end(600, 2);
}

unsigned char sim868_command_responce_http_para(const char* command, const char* responce)
{
	sim868_wait_responce_begin( responce );
	
	sim868_print_progmem( sim868_TextHttp );
	sim868_print_progmem( sim868_TextPara );
	sim868_print_progmem( command);
	
	return sim868_wait_responce_end(150, 2);
}

void sim868_wait_responce_begin (const char* responce)
{
	_delay_ms(100);
	
	sim868_responce = responce;
	sim868_responce_pointer = 0;
	sim868_responce_line = 0;
	
	for(sim868_responce_len=0; (char)(pgm_read_byte( &responce[ sim868_responce_len ] )); sim868_responce_len++);
	
	sim868_print_progmem( sim868_data__at_plus );	
}

unsigned char sim868_wait_responce_end (unsigned int timeout, unsigned char lineout)
{
	sim868_responce_buf_len = 0;
	sim868_responce_write_pointer_begin = 0;
	sim868_print_newstr();
	
	unsigned int responce_buf_len_prev;
	unsigned int responce_buf_p = 0;
	unsigned int responce_buf_line_count = 0;
	unsigned int responce_buf_lineout = lineout;
	unsigned int tick = 0;
	
	while( tick++ < timeout )
	{
		if( (responce_buf_line_count >= responce_buf_lineout ) ||
			(sim868_responce_buf_len >= sim868_responce_buf_len_max) )
		{
			responce_buf_len_prev = sim868_responce_buf_len;
			break;
		}
		
		if( responce_buf_len_prev != sim868_responce_buf_len ) 
		{
			responce_buf_len_prev = sim868_responce_buf_len;
			tick = 0;
			
			while( responce_buf_p < responce_buf_len_prev ) 
			{
				if( sim868_responce_buf[ responce_buf_p++ ] == '\n')
				{
					responce_buf_line_count++;
				}
			}			
		}
		
		_delay_ms( SIM868_TIMEOUT_TICK );
	}
	
	
	sim868_responce_pointer = 0;
	unsigned char char_a;
	unsigned char char_b;
	
	for( unsigned int i=0; i<sim868_responce_buf_len; i++)
	{
		char_a = sim868_responce_buf[i];
		char_b = pgm_read_byte( &sim868_responce[ sim868_responce_pointer ] );
		if( char_a == char_b )
		{
			sim868_responce_pointer++;
			if( sim868_responce_pointer >= sim868_responce_len )
			{
				sim868_responce_write_pointer_begin = i;
				sim868_responce_write_pointer_end = sim868_responce_write_pointer_begin;
				return GOOD_CODE;
			}
		}
		else
		{
			sim868_responce_pointer = 0;			
		}
	}
	
	return ERROR_CODE;
}

void sim868_power_en(void)
{
	_delay_ms(1000);
	
	if( (sim868_command_responce(sim868_command__at, sim868_data__error) == GOOD_CODE) ||
		(sim868_command_responce(sim868_command__at, sim868_data__error) == GOOD_CODE) ||
		(sim868_command_responce(sim868_command__at, sim868_data__error) == GOOD_CODE) ||
		(sim868_command_responce(sim868_command__at, sim868_data__error) == GOOD_CODE) ) 
	{		
		sim868_command_responce( sim868_command__gnss_power_on, sim868_data__ok );
		sim868_command_responce( sim868_command__gnss_filter_rmc, sim868_data__ok );
		
		return;
	}
	
	pin_output( SIM868_EN_PIN );
	
	pin_high( SIM868_EN_PIN );
	_delay_ms(1000);
	pin_low( SIM868_EN_PIN );
	_delay_ms(1000);
	pin_high( SIM868_EN_PIN );
	_delay_ms(5000);
	
	sim868_print_progmem( sim868_command__at );
	sim868_print_newstr();
	_delay_ms(5000);
	
	sim868_print_progmem( sim868_command__at );
	sim868_print_newstr();
	_delay_ms(3000);
	
	
	sim868_command_responce( sim868_command__at, sim868_data__error );
	sim868_command_responce( sim868_command__gnss_power_on, sim868_data__ok );
	sim868_command_responce( sim868_command__gnss_filter_rmc, sim868_data__ok );
	_delay_ms(100);
	
	if( (sim868_command_responce(sim868_command__at, sim868_data__error) == GOOD_CODE) ||
	(sim868_command_responce(sim868_command__at, sim868_data__error) == GOOD_CODE) ||
	(sim868_command_responce(sim868_command__at, sim868_data__error) == GOOD_CODE) ||
	(sim868_command_responce(sim868_command__at, sim868_data__error) == GOOD_CODE) )
	{
		sim868_command_responce( sim868_command__gnss_power_on, sim868_data__ok );
		sim868_command_responce( sim868_command__gnss_filter_rmc, sim868_data__ok );
		
		return;
	}
	
	pin_output( SIM868_EN_PIN );
	
	pin_high( SIM868_EN_PIN );
	_delay_ms(1000);
	pin_low( SIM868_EN_PIN );
	_delay_ms(1000);
	pin_high( SIM868_EN_PIN );
	_delay_ms(5000);
	
	sim868_print_progmem( sim868_command__at );
	sim868_print_newstr();
	_delay_ms(5000);
	
	sim868_print_progmem( sim868_command__at );
	sim868_print_newstr();
	_delay_ms(500);
	
	sim868_command_responce( sim868_command__at, sim868_data__error );
	sim868_command_responce( sim868_command__gnss_power_on, sim868_data__ok );
	sim868_command_responce( sim868_command__gnss_filter_rmc, sim868_data__ok );
	_delay_ms(100);
	
}

void sim868_power_dis(void)
{
	sim868_print_progmem( sim868_data__at_plus );
	sim868_print_progmem( sim868_command__power_down );
	sim868_print_progmem( sim868_data__en );
	sim868_print_newstr();
	_delay_ms(1000);
}

void sim868_delay(unsigned int delay_time)
{
	//_delay_ms( delay_time );
	unsigned int i;
	unsigned int j = delay_time;
	
	while(j--)
	{
		i = SIM868_DELAY_TICK_MS; 
		while(i--) asm("NOP");
	}
}

void sim868_buffer_print(char *buffer, unsigned int start_point, unsigned int end_point)
{
	for( unsigned int i=start_point; i<end_point; i++ )
	{
		sim868_print_char( buffer[i] );
	}
}





void sim868_update(void)
{
	_delay_ms(3000);
}



ISR (usart_interrupt_vector)
{
	usart_received_byte_get( sim868_readed_char );
	if( sim868_responce_buf_len < sim868_responce_buf_len_max )
	{
		sim868_responce_buf[ sim868_responce_buf_len++ ] = sim868_readed_char;
	}	
}



void sim868_init(void)
{
	interrupts_global_dis();
	
	usart_reset_full();
	while( usart_busy_get() );
	
	usart_regs_clr();
	
	usart_baudrate_put( SIM868_BAUDRATE );
	
	usart_transmitter_ports_init();
	usart_transmitter_en();
	//usart_transmitted_intr_en();
	
	usart_receiver_ports_init();
	usart_receiver_en();
	usart_received_intr_en();
	
	usart_en();
	while( usart_busy_get() );
		
	interrupts_global_en();
	
	_delay_ms(1000);
	
	sim868_power_en();
}





void sim868_get_char(char *data)
{
	usart_received_byte_get( *data );
}

void sim868_print_char(char data)
{
	usart_transmite_byte_put( data );
	while( !usart_transmitted_get() );
}



void sim868_print_newstr(void)
{
	sim868_print_char('\n');
	//sim868_print_char('\r');
}

void sim868_print_progmem(const char* data)
{
	for(unsigned int i=0; (char)pgm_read_byte( &data[i] ); i++)
	{
		sim868_print_char(  (char) pgm_read_byte( &data[i] )  );
	}
}

void sim868_print_chararr(char* data)
{
	for(unsigned int i=0; data[i]; i++)
	{
		sim868_print_char( data[i] );
	}
}

void sim868_print_chararr_by_len( char* data, unsigned int len )
{
	for(unsigned int i=0; i < len; i++)
	{
		sim868_print_char( data[i] );
	}
}

void sim868_print_uint(unsigned int numb)
{
	if (numb==0)
	{
		sim868_print_char(48);
	}
	else
	{
		unsigned int a = 1;
		unsigned char len = 0;
		
		while(numb + 1 > a)
		{
			len++; a*=10;
		}
		
		if (len==5)
		{
			a=10000;
		}
		else
		{
			a/=10;
		}
		
		for(unsigned char dig=0; dig<len; dig++ )
		{
			sim868_print_char(48+numb/a%10);
			a/=10;
		}
	}
}