/*
 * sim868_config.h
 *
 * Created: 20/01/2019 16:25:29
 *  Author: Danil Murashkin
 */ 


#ifndef SIM868_CONFIG_H_
#define SIM868_CONFIG_H_

#ifdef	__cplusplus
extern "C" {
#endif


	
	#define SIM868_BAUDRATE			9600
	
	#define SIM868_EN_PIN			B,5
	
	#define SIM868_BUFFER_SIZE		255
	#define SIM868_DELAY_TICK_MS	400
	#define SIM868_TIMEOUT_TICK		4
	


#ifdef	__cplusplus
}
#endif

#endif //SIM868_CONFIG_H_ 