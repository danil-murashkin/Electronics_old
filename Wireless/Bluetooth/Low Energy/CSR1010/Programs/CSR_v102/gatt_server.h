/******************************************************************************
 *  Copyright (c) 2017 bluePay
 *  Part of CSR uEnergy SDK 2.6.0
 *  Application version 2.6.0.0
 *
 *  FILE
 *      gatt_server.h
 *
 *  DESCRIPTION
 *      Header file for a simple GATT server application.
 *
 ******************************************************************************/

#ifndef __GATT_SERVER_H__
#define __GATT_SERVER_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>          /* Commonly used type definitions */

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "gatt_access.h"    /* GATT-related routines */
/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Maximum number of words in central device Identity Resolving Key (IRK) */
#define MAX_WORDS_IRK                       (8)

#define CMDD_PARAMETERS_MAX 15
#define CMDD_PARAMETER_MAX_LENGHT 20
#define CMDD_DATA_MAX_LENGHT 500
uint8  Cmdd_Parameter[CMDD_PARAMETER_MAX_LENGHT];
uint16 Cmdd_Parameter_Pointer;
uint8  Cmdd_Data[CMDD_DATA_MAX_LENGHT];
uint16 Cmdd_Data_Pointer;



#define TEXT_SYMBOL_LEN 1
static uint8  TextBracketFLeft[1] 		= {'{'};
static uint8  TextBracketFRight[1] 		= {'}'};
static uint8  TextApostropheD[1] 		= {'\"'};
static uint8  TextComma[1] 				= {','};
static uint8  TextDotD[1] 				= {':'};
#define TEXT_NEW_STR_LEN 2
static uint8  TextNewStr[TEXT_NEW_STR_LEN] 				= {'\r','\n'};
#define TEXT_APOSTROPHE_D_COMA_LEN 4
static uint8  TextApostropheDComa[TEXT_APOSTROPHE_D_COMA_LEN] 			= {'\"',',','\r','\n'};
//static uint8  TextApostropheDComa[TEXT_APOSTROPHE_D_COMA_LEN] 			= {'\"',','};


unsigned char BleStatusEnableFlag;
#define BLE_OBJ_LEN 10
#define BLE_DATA_LEN 6
#define BLE_OBJ_SECURE 4
static uint8  BleObj[BLE_OBJ_LEN] 				= {'b','l','e', '_', 'e','n','a','b','l','e'};
	   uint8  BleData[BLE_DATA_LEN];
	   uint16 BleDataLenght;
	   uint8  BleEnableFlag;

//INPUT
#define HOST_LEN 4
#define HOST_DATA_MAX_LEN 50
#define HOST_SECURE 4
static uint8  Host[HOST_LEN] 				= {'h','o','s','t'};
	   uint8  HostData[HOST_DATA_MAX_LEN];
	   uint16 HostDataLenght;

#define PATH_LEN 4
#define PATH_DATA_MAX_LEN 150
#define PATH_SECURE 4
static uint8  Path[PATH_LEN] 				= {'p','a','t','h'};
	   uint8  PathData[PATH_DATA_MAX_LEN];
	   uint16 PathDataLenght;

#define MESSAGE_LEN 7
#define MESSAGE_DATA_MAX_LEN 500
#define MESSAGE_SECURE 4
static uint8  Message[MESSAGE_LEN] 				= {'m','e','s','s','a','g','e'};
	   uint8  MessageData[MESSAGE_DATA_MAX_LEN];
	   uint16 MessageDataLenght;

#define AMOUNT_LEN 6
#define AMOUNT_DATA_MAX_LEN 6
#define AMOUNT_SECURE 1
static uint8  Amount[AMOUNT_LEN] 				= {'a','m','o','u','n','t'};
	   uint8  AmountData[AMOUNT_DATA_MAX_LEN];
	   uint16 AmountDataLenght;
	   uint8  AmountDataFlag;

#define NAME_LEN 8
#define NAME_DATA_MAX_LEN 30
#define NAME_SECURE 1
static uint8  Name[NAME_LEN] 				= {'b','l','e','_','n','a','m','e'};
	   uint8  NameData[NAME_DATA_MAX_LEN];
	   uint16 NameDataLenght;




//OUTPUT
#define TEXT_URL_LEN 7
static uint8  TextUrl[TEXT_URL_LEN] 			= {'\"','u','r','l','\"',':','\"'};
#define TEXT_URL_HTTPS_LEN 7
static uint8  TextUrlHttps[TEXT_URL_HTTPS_LEN] 	= {'h','t','t','p',':','/','/'};

#define TEXT_POST_LEN 8
static uint8  TextPost[TEXT_POST_LEN] 			= {'\"','p','o','s','t','\"',':','{'};

#define TEXT_POST_AMOUNT_LEN 9
static uint8  TextPostAmount[TEXT_POST_AMOUNT_LEN] 		= {'\"','a','m','o','u','n','t','\"',':'};

#define TEXT_DEVICE_ID_LEN 12
static uint8  TextDevicelId[TEXT_DEVICE_ID_LEN]	= {'\"','d','e','v','i','c','e','I','d','\"',':','\"'};
#define DEVICE_ID_DATA_LEN 5
static uint8  DeviceIdData[DEVICE_ID_DATA_LEN]	= {'1','0','1','0','1'};

#define TEXT_MESSAGE_LEN 20
static uint8  TextMessage[TEXT_MESSAGE_LEN]	= {'\"','m','o','b','i','l','e','A','p','p','M','e','s','s','a','g','e','\"',':','\"'};


#define TEXT_DPKB64_LEN 29
static uint8  TextDevicePublicKeyBase64[TEXT_DPKB64_LEN]	= {'\"','d','e','v','i','c','e','P','u','b','l','i','c','K','e','y','B','a','s','e','6','4','\"',':','\"','\"',',','\r','\n'};

#define TEXT_DPKS_LEN 32
static uint8  TextDevicePublicKeySignature[TEXT_DPKS_LEN]	= {'\"','d','e','v','i','c','e','P','u','b','l','i','c','K','e','y','S','i','g','n','a','t','u','r','e','\"',':','\"','\"',',','\r','\n'};

#define TEXT_B64S_LEN 22
static uint8  TextBase64Signature[TEXT_B64S_LEN]	= {'\"','b','a','s','e','6','4','S','i','g','n','a','t','u','r','e','\"',':','\"','\"','\r','\n'};




uint8  Parameters_Data_Buf[CMDD_PARAMETERS_MAX][CMDD_DATA_MAX_LENGHT];
uint8  Parameters_Secure[CMDD_PARAMETERS_MAX];


#define REQUEST_BUF_SIZE 1500
#define REQUEST_ATTEMPTS_CLEAR 2
uint8  Request_Buf[REQUEST_BUF_SIZE];
uint16 Request_Buf_Pointer;
uint16 Request_Read_Flag;
uint8  Request_Read_Attempts;

#define RESPONCE_BUF_SIZE 1500
uint8  Responce_Buf[RESPONCE_BUF_SIZE];
uint16 Responce_Buf_Lenght;

#define WIFI_BUF_SIZE 1500
uint8  WiFi_Buf[WIFI_BUF_SIZE];
uint16 WiFi_Buf_Pointer;

#define DATA_BUF_SIZE 1000
#define DATA_ATTEMPTS_CLEAR 2
uint8  Data_Write_Buf[DATA_BUF_SIZE];
uint16 Data_Write_Buf_Lenght;
uint8  Data_Write_Buf_Flag;
uint16 Data_Write_Buf_Chunk_Lenght;
uint16 Data_Write_Buf_Chunk_Pointer;

uint8  Data_Read_Buf[DATA_BUF_SIZE];
uint16 Data_Read_Buf_Lenght;
uint8  Data_Read_Buf_Flag;
uint16 Data_Read_Buf_Chunk_Lenght;
uint16 Data_Read_Buf_Chunk_Pointer;


#define DATA_CHUNK_LENGHT 20
#define DATA_UINT_LEN 5
uint8 Data_Chunk_Buf[DATA_CHUNK_LENGHT];

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/
extern void ArrayClear(uint8* data, uint16 data_lenght);
extern void ArrayPrint(uint8* data, uint16 data_lenght);
extern void ArrayPut(uint8* array, uint16 *array_start, uint8* data, uint16 data_lenght);
extern void ArrayPutArray(uint8* array, uint16 array_lenght, uint8* data, uint16 data_lenght, uint16 array_start, uint16 data_start, uint16 put_lenght);

extern uint16 StrToInt(uint8* data, uint16 data_lenght);
extern void IntToStr(uint16 num, uint8* data, uint8 *data_lenght);

extern uint8 UsartDataParce(uint8* request, uint16 request_lenght, uint8* responce, uint16 *responce_lenght);
extern uint8 BluetoothDataParce(uint8* request, uint16 request_lenght, uint8* responce, uint16 *responce_lenght);
extern uint8 FindParameter(uint8* request, uint16 request_lenght, uint8 secure_code, uint8* param, uint16 param_len, uint8 param_secure, uint8* param_data, uint16 *param_data_len);
extern uint8 CheckParameterData(uint8* data, uint16 data_len, uint8* check_data, uint16 check_data_len);
extern uint8 CheckParameter( uint8* param, uint16 param_len, uint8* param_data, uint16 *param_data_len, uint8* chk_param, uint16 chk_param_len, uint8* chk_data, uint16 chk_data_len);


/* Call the firmware Panic() routine and provide a single point for debugging
 * any application level panics
 */
extern void ReportPanic(app_panic_code panic_code);

/* Handle a short button press. If connected, the device disconnects from the
 * host, otherwise it starts advertising.
 */
extern void HandleShortButtonPress(void);

/* Change the current state of the application */
extern void SetState(app_state new_state);

/* Return the current state of the application.*/
extern app_state GetState(void);

/* Check if the whitelist is enabled or not. */
extern bool IsWhiteListEnabled(void);

/* Handle pairing removal */
extern void HandlePairingRemoval(void);

/* Start the advertisement timer. */
extern void StartAdvertTimer(uint32 interval);

/* Return whether the connected device is bonded or not */
extern bool IsDeviceBonded(void);

/* Return the unique connection ID (UCID) of the connection */
extern uint16 GetConnectionID(void);

#endif /* __GATT_SERVER_H__ */
