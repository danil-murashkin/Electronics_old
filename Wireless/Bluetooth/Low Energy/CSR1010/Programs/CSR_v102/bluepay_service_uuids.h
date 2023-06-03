/******************************************************************************
 *  Copyright (c) 2017 bluePay
 *  Part of CSR uEnergy SDK 2.6.0
 *  Application version 2.6.0.0
 *
 *  FILE
 *      bluepay_service_uuids.h
 *
 *  DESCRIPTION
 *      UUID MACROs for the bluePay service
 *
 *****************************************************************************/

#ifndef __BLUEPAY_SERVICE_UUIDS_H__
#define __BLUEPAY_SERVICE_UUIDS_H__

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Brackets should not be used around the values of these macros. This file is
 * imported by the GATT Database Generator (gattdbgen) which does not understand 
 * brackets and will raise syntax errors.
 */

/* For UUID values, refer http://developer.bluetooth.org/gatt/services/
 * Pages/ServiceViewer.aspx?u=org.bluetooth.service.device_information.xml
 */

#define UUID_BLUEPAY_SERVICE 				0x1811 //0x183A

#define UUID_BLUEPAY_SERVER_STATUS			0x3A01
#define UUID_BLUEPAY_CLIENT_STATUS			0x3A02

#define UUID_BLUEPAY_VALUE_TOKEN			0x3A10

#define UUID_BLUEPAY_ID						0x3A21
#define UUID_BLUEPAY_TYPE					0x3A22

#define UUID_BLUEPAY_DATA_READ 				0x3A61
#define UUID_BLUEPAY_DATA_READ_POINTER  	0x3A62
#define UUID_BLUEPAY_DATA_READ_LENGHT		0x3A63
#define UUID_BLUEPAY_DATA_WRITE			    0x3A71
#define UUID_BLUEPAY_DATA_WRITE_POINTER  	0x3A72
#define UUID_BLUEPAY_DATA_WRITE_LENGHT 		0x3A73

#endif /* __BLUEPAY_SERVICE_UUIDS_H__ */
