/*****************************************************************************
 *  Copyright (c) 2017 bluePay
 *  Part of CSR uEnergy SDK 2.6.0
 *  Application version 2.6.0.0
 *
 *  FILE
 *      leds.c
 *
 *  DESCRIPTION
 *      
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header files
 *============================================================================*/
#include <pio.h>           /* GATT application interface */

/*============================================================================*
 *  Local Header files
 *============================================================================*/
#include "leds.h" 

/*============================================================================*
 *  Private Definitions
 *============================================================================*/
#define PIO_RED        9
#define PIO_GREEN      10 
#define PIO_BLUE       11

#define PIO_DIR_OUTPUT  TRUE        /* PIO direction configured as output */
#define PIO_DIR_INPUT   FALSE       /* PIO direction configured as input */

/*============================================================================*
 *  Private Datatypes
 *============================================================================*/


/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/
extern void LedsInit(void)
{
    PioSetModes((1UL << PIO_RED) | (1UL << PIO_GREEN) | (1UL << PIO_BLUE), pio_mode_user);

    PioSetDir(PIO_RED, PIO_DIR_OUTPUT);
    PioSetDir(PIO_GREEN, PIO_DIR_OUTPUT);
    PioSetDir(PIO_BLUE, PIO_DIR_OUTPUT);

    PioSet(PIO_RED, 1);
    PioSet(PIO_GREEN, 1);
    PioSet(PIO_BLUE, 1);
}

extern void LedsGreenOn(void)
{
    PioSet(PIO_RED, 1);
    PioSet(PIO_GREEN, 0);
    PioSet(PIO_BLUE, 1);
}

extern void LedsRedOn(void)
{
    PioSet(PIO_RED, 0);
    PioSet(PIO_GREEN, 1);
    PioSet(PIO_BLUE, 1);
}

extern void LedsBlueOn(void)
{
    PioSet(PIO_RED, 1);
    PioSet(PIO_GREEN, 1);
    PioSet(PIO_BLUE, 0);
}