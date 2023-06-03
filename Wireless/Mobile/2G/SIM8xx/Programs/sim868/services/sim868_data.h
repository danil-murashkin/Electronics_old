/*
 * sim868_868_data.h
 *
 * Created: 20/01/2019 21:12:20
 *  Author: danil
 */ 


#ifndef sim868_868_DATA_H_
#define sim868_868_DATA_H_

#ifdef	__cplusplus
extern "C" {
#endif


	
	#define SIM868_HTTPREAD_HEADER_LEN		26

	const char sim868_data__at_plus[]				PROGMEM = "AT+";
	const char sim868_data__en[]					PROGMEM = "=1";
	const char sim868_data__dis[]					PROGMEM = "=0";
	const char sim868_data__get_status[]			PROGMEM = "?";	
	const char sim868_data__ok[]					PROGMEM = "OK";
	const char sim868_data__error[]					PROGMEM = "ERROR";	
	const char sim868_data__comma[]					PROGMEM = ", ";
	const char sim868_data__space[]					PROGMEM = " ";
	const char sim868_data__newstr[]				PROGMEM = "\r\n";
	const char sim868_data__null[]					PROGMEM = "";
	const char sim868_data__space_10[]				PROGMEM = "          ";
	
	const char sim868_command__at[]					PROGMEM = "AT";
	const char sim868_command__power_down[]			PROGMEM = "CPOWD";
	const char sim868_command__gnss_power[]			PROGMEM = "CGNSPWR";	
	const char sim868_command__gnss_info[]			PROGMEM = "CGNSINF";
	const char sim868_command__gnss_filter[]		PROGMEM = "CGNSSEQ";
		const char sim868_data__gnss_filter_rmc[]		PROGMEM = "RMC";
		
	const char sim868_command__gnss_power_on[]		PROGMEM = "CGNSPWR=1";
	const char sim868_command__gnss_filter_rmc[]	PROGMEM = "CGNSSEQ=\"RMC\"";
	const char sim868_command__gnss_get_info[]		PROGMEM = "CGNSINF";
	const char sim868_data__gnss_get_info[]			PROGMEM = "CGNSINF: ";
	const char sim868_command__gnss_nmea_log_en[]	PROGMEM = "CGNSTST=1";	
	const char sim868_CmdCReg[]						PROGMEM = "CREG?";
	const char sim868_CmdCRegResp[]					PROGMEM = "CREG: ";
	const char sim868_CmdSapbr31Gprs[]				PROGMEM = "SAPBR=3,1,\"CONTYPE\",\"GPRS\"";
	const char sim868_CmdSapbrGprs11[]				PROGMEM = "SAPBR=1,1";
	const char sim868_CmdSapbrGprs01[]				PROGMEM = "SAPBR=0,1";
	const char sim868_TextHttp[]					PROGMEM = "HTTP";
	const char sim868_CmdHttpInit[]					PROGMEM = "INIT";
	const char sim868_CmdHttpTerm[]					PROGMEM = "TERM";
	const char sim868_TextPara[]					PROGMEM = "PARA=\"";
	const char sim868_CmdHttpParaCid1[]				PROGMEM = "CID\",1";
	const char sim868_CmdHttpParaUrl[]				PROGMEM = "URL\",\"";
	const char sim868_CmdHttpParaUrlEnd[]			PROGMEM = "\"";
	const char sim868_CmdHttpParaContApl[]			PROGMEM = "CONTENT\",\"application/x-www-form-urlencoded\"";
	const char sim868_CmdHttpAct[]					PROGMEM = "ACTION=1";
	const char sim868_RespHttpAct200[]				PROGMEM = "ACTION: 1,200,";
	const char sim868_CmdHttpRead[]					PROGMEM = "READ";
	const char sim868_RespHttpRead[]				PROGMEM = "READ: ";
	const char sim868_CmdHttpData[]					PROGMEM = "DATA=";
	const char sim868_HttpDataDelay[]				PROGMEM = ",100000";
	const char sim868_HttpRespDownload[]			PROGMEM = "DOWNLOAD";
	const char sim868_HttpRespAllOk[]				PROGMEM = "ALL-OK";
		
	
		
#ifdef	__cplusplus
}
#endif

#endif //sim868_868_DATA_H_