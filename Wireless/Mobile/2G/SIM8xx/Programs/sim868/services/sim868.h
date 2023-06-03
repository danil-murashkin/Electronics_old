/*
 * sim868.h
 *
 * Created: 20/01/2019 16:20:36
 *  Author: Danil Murashkin
 */ 


#ifndef SIM868_H_
#define SIM868_H_

#ifdef	__cplusplus
extern "C" {
#endif


	
	#include "../config/sim868_config.h"
	char sim868_buffer[ SIM868_BUFFER_SIZE ];
		
		
	void sim868_init(void);
	void sim868_update(void);
	void sim868_example_request(void);
	
	unsigned char sim868_request_get_send( const char* host, const char* path, const char* params, unsigned int *responce_len );
	unsigned char sim868_request_get_end(void);
	
	unsigned char sim868_get_location( char* latitude, unsigned char* latitude_len, char* longtitude, unsigned char* longtitude_len );
	
	void sim868_print_newstr(void);
	void sim868_print_progmem( const char* data );
	void sim868_print_chararr( char* data );
	void sim868_print_chararr_by_len( char* data, unsigned int len );
	void sim868_print_uint( unsigned int numb );
	unsigned int sim868_buffer_to_uint( char *buffer_data, unsigned int start_pointer, unsigned int end_pointer );
	
	
	
#ifdef	__cplusplus
}
#endif

#endif //SIM868_H_