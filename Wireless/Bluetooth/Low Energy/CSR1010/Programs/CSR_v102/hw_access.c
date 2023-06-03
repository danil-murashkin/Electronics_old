/******************************************************************************
 *  Copyright (c) 2017 bluePay
 *  Part of CSR uEnergy SDK 2.6.0
 *  Application version 2.6.0.0
 *
 *  FILE
 *      hw_access.c
 *
 *  DESCRIPTION
 *      This file defines the application hardware specific routines.
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <pio.h>            /* PIO configuration and control functions */
#include <pio_ctrlr.h>      /* Access to the PIO controller */
#include <timer.h>          /* Chip timer functions */

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "hw_access.h"      /* Interface to this file */
#include "gatt_server.h"    /* Definitions used throughout the GATT server */

/*============================================================================*
 *  Private Definitions
 *============================================================================*/


/*============================================================================*
 *  Public data type
 *============================================================================*/


/*============================================================================*
 *  Public data
 *============================================================================*/


/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/


/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/


/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      InitHardware
 *
 *  DESCRIPTION
 *      This function is called to initialise the application hardware.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void InitHardware(void)
{
    /* Setup PIOs
     */

    /* Save power by changing the I2C pull mode to pull down.*/
    PioSetI2CPullMode(pio_i2c_pull_mode_strong_pull_down);
    
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HwDataInit
 *
 *  DESCRIPTION
 *      This function initialises the hardware data to a known state. It is
 *      intended to be called once, for example after a power-on reset or HCI
 *      reset.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void HwDataInit(void)
{

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HwDataReset
 *
 *  DESCRIPTION
 *      This function resets the hardware data. It is intended to be called when
 *      the data needs to be reset to a clean state, for example, whenever a
 *      device connects or disconnects.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void HwDataReset(void)
{
    
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HandlePIOChangedEvent
 *
 *  DESCRIPTION
 *      This function handles the PIO Changed event.
 *
 *  PARAMETERS
 *      pio_data [in]           State of the PIOs when the event occurred
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void HandlePIOChangedEvent(pio_changed_data *pio_data)
{

   
}

