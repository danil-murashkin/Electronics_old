/*****************************************************************************
 *  Copyright (c) 2017 bluePay
 *  Part of CSR uEnergy SDK 2.6.0
 *  Application version 2.6.0.0
 *
 *  FILE
 *      bluepay_service_.h
 *
 *  DESCRIPTION
 *      Header definitions for the bluePay service
 *
 *****************************************************************************/

#ifndef __BLUEPAY_SERVICE_H__
#define __BLUEPAY_SERVICE_H__

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* Handle write operations on bluePay attributes maintained by the application*/
extern void BluePayHandleAccessWrite(GATT_ACCESS_IND_T *p_ind);

/* Handle read operations on bluePay attributes maintained by the application*/
extern void BluePayHandleAccessRead(GATT_ACCESS_IND_T *p_ind);

/* Check if the handle belongs to the Device Information Service */
extern bool BluePayCheckHandleRange(uint16 handle);

#endif /*__BLUEPAY_SERVICE_H__*/
