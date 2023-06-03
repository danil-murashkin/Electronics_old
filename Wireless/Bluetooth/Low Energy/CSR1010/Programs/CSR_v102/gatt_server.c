#define ENABLE_FOLD
#ifdef ENABLE_FOLD 
	/******************************************************************************
	 *  Copyright (c) 2017 bluePay
	 *  Part of CSR uEnergy SDK 2.6.0
	 *  Application version 2.6.0.0
	 *
	 *  FILE
	 *      gatt_server.c
	 *
	 *  DESCRIPTION
	 *      This file defines a simple implementation of a GATT server.
	 *

	/*============================================================================*
	 *  SDK Header Files
	 *============================================================================*/

	#include <main.h>           /* Functions relating to powering up the device */
	#include <types.h>          /* Commonly used type definitions */
	#include <timer.h>          /* Chip timer functions */
	#include <mem.h>            /* Memory library */
	#include <config_store.h>   /* Interface to the Configuration Store */
	/*******************************************************************************/
	#include <bt_event_types.h>          /* Commonly used type definitions */
	#include <debug.h>

	/* Upper Stack API */
	#include <gatt.h>           /* GATT application interface */
	#include <ls_app_if.h>      /* Link Supervisor application interface */
	#include <gap_app_if.h>     /* GAP application interface */
	#include <buf_utils.h>      /* Buffer functions */
	#include <security.h>       /* Security Manager application interface */
	#include <panic.h>          /* Support for applications to panic */
	#include <nvm.h>            /* Access to Non-Volatile Memory */
	#include <random.h>         /* Generators for pseudo-random data sequences */

	/*============================================================================*
	 *  Local Header Files
	 *============================================================================*/
	                                
	#include "gatt_access.h"    /* GATT-related routines */
	#include "app_gatt_db.h"    /* GATT database definitions */
	#include "nvm_access.h"     /* Non-volatile memory access */
	#include "gatt_server.h"    /* Definitions used throughout the GATT server */
	#include "hw_access.h"      /* Hardware access */
	#include "debug_interface.h"/* Application debug routines */
	#include "gap_service.h"    /* GAP service interface */
	#include "bluepay_service.h"/* bluePay service interface */
	#include "leds.h"

	#include "gap_service.h"

	/*============================================================================*
	 *  Private Definitions
	 *============================================================================*/

	/* Maximum number of timers. Up to five timers are required by this application:
	 *  
	 *  buzzer.c:       buzzer_tid
	 *  This file:      con_param_update_tid
	 *  This file:      app_tid
	 *  This file:      bonding_reattempt_tid (if PAIRING_SUPPORT defined)
	 *  hw_access.c:    button_press_tid
	 */
	#define MAX_APP_TIMERS                 (5)

	/* Number of Identity Resolving Keys (IRKs) that application can store */
	#define MAX_NUMBER_IRK_STORED          (1)

	/* Magic value to check the sanity of Non-Volatile Memory (NVM) region used by
	 * the application. This value is unique for each application.
	 */
	#define NVM_SANITY_MAGIC               (0xABAA)

	/* NVM offset for NVM sanity word */
	#define NVM_OFFSET_SANITY_WORD         (0)

	/* NVM offset for bonded flag */
	#define NVM_OFFSET_BONDED_FLAG         (NVM_OFFSET_SANITY_WORD + 1)

	/* NVM offset for bonded device Bluetooth address */
	#define NVM_OFFSET_BONDED_ADDR         (NVM_OFFSET_BONDED_FLAG + \
	                                        sizeof(g_app_data.bonded))

	/* NVM offset for diversifier */
	#define NVM_OFFSET_SM_DIV              (NVM_OFFSET_BONDED_ADDR + \
	                                        sizeof(g_app_data.bonded_bd_addr))

	/* NVM offset for IRK */
	#define NVM_OFFSET_SM_IRK              (NVM_OFFSET_SM_DIV + \
	                                        sizeof(g_app_data.diversifier))

	/* Number of words of NVM used by application. Memory used by supported 
	 * services is not taken into consideration here.
	 */
	#define NVM_MAX_APP_MEMORY_WORDS       (NVM_OFFSET_SM_IRK + \
	                                        MAX_WORDS_IRK)

	/* Slave device is not allowed to transmit another Connection Parameter 
	 * Update request till time TGAP(conn_param_timeout). Refer to section 9.3.9.2,
	 * Vol 3, Part C of the Core 4.0 BT spec. The application should retry the 
	 * 'Connection Parameter Update' procedure after time TGAP(conn_param_timeout)
	 * which is 30 seconds.
	 */
	#define GAP_CONN_PARAM_TIMEOUT          (30 * SECOND)

	/*============================================================================*
	 *  Private Data types
	 *============================================================================*/

	/* Application data structure */
	typedef struct _APP_DATA_T
	{
	    /* Current state of application */
	    app_state                  state;

	    /* TYPED_BD_ADDR_T of the host to which device is connected */
	    TYPED_BD_ADDR_T            con_bd_addr;

	    /* Track the Connection Identifier (UCID) as Clients connect and
	     * disconnect
	     */
	    uint16                     st_ucid;

	    /* Boolean flag to indicate whether the device is bonded */
	    bool                       bonded;

	    /* TYPED_BD_ADDR_T of the host to which device is bonded */
	    TYPED_BD_ADDR_T            bonded_bd_addr;

	    /* Diversifier associated with the Long Term Key (LTK) of the bonded
	     * device
	     */
	    uint16                     diversifier;

	    /* Timer ID for Connection Parameter Update timer in Connected state */
	    timer_id                   con_param_update_tid;

	    /* Central Private Address Resolution IRK. Will only be used when
	     * central device used resolvable random address.
	     */
	    uint16                     irk[MAX_WORDS_IRK];

	    /* Number of connection parameter update requests made */
	    uint8                      num_conn_update_req;


	    /* Timer ID for 'UNDIRECTED ADVERTS' and activity on the sensor device like
	     * measurements or user intervention in CONNECTED state.
	     */
	    timer_id                   app_tid;

	    /* Boolean flag to indicate whether to set white list with the bonded
	     * device. This flag is used in an interim basis while configuring 
	     * advertisements.
	     */
	    bool                       enable_white_list;

	    /* Current connection interval */
	    uint16                     conn_interval;

	    /* Current slave latency */
	    uint16                     conn_latency;

	    /* Current connection timeout value */
	    uint16                     conn_timeout;
	} APP_DATA_T;

	/*============================================================================*
	 *  Private Data
	 *============================================================================*/

	/* Declare space for application timers */
	static uint16 app_timers[SIZEOF_APP_TIMER * MAX_APP_TIMERS];

	/* Application data instance */
	static APP_DATA_T g_app_data;

	/*============================================================================*
	 *  Private Function Prototypes
	 *============================================================================*/

	/* Initialise application data structure */
	static void appDataInit(void);

	/* Initialise and read NVM data */
	static void readPersistentStore(void);

	/* Enable whitelist based advertising */
	static void enableWhiteList(void);

	/* Start the Connection update timer */
	static void appStartConnUpdateTimer(void);

	/* Send L2CAP_CONNECTION_PARAMETER_UPDATE_REQUEST to the remote device */
	static void requestConnParamUpdate(timer_id tid);

	/* Exit the advertising states */
	static void appExitAdvertising(void);

	/* Exit the initialisation state */
	static void appInitExit(void);

	/* Handle advertising timer expiry */
	static void appAdvertTimerHandler(timer_id tid);



	/* LM_EV_CONNECTION_COMPLETE signal handler */
	static void handleSignalLmEvConnectionComplete(LM_EV_CONNECTION_COMPLETE_T *p_event_data);

	/* GATT_ADD_DB_CFM signal handler */
	static void handleSignalGattAddDbCfm(GATT_ADD_DB_CFM_T *p_event_data);

	/* GATT_CANCEL_CONNECT_CFM signal handler */
	static void handleSignalGattCancelConnectCfm(void);

	/* GATT_CONNECT_CFM signal handler */
	static void handleSignalGattConnectCfm(GATT_CONNECT_CFM_T *p_event_data);

	/* SM_KEYS_IND signal handler */
	static void handleSignalSmKeysInd(SM_KEYS_IND_T *p_event_data);

	/* SM_SIMPLE_PAIRING_COMPLETE_IND signal handler */
	static void handleSignalSmSimplePairingCompleteInd(SM_SIMPLE_PAIRING_COMPLETE_IND_T *p_event_data);

	/* SM_DIV_APPROVE_IND signal handler */
	static void handleSignalSmDivApproveInd(SM_DIV_APPROVE_IND_T *p_event_data);

	/* LS_CONNECTION_PARAM_UPDATE_CFM signal handler */
	static void handleSignalLsConnParamUpdateCfm(LS_CONNECTION_PARAM_UPDATE_CFM_T *p_event_data);

	/* LS_CONNECTION_PARAM_UPDATE_IND signal handler */
	static void handleSignalLsConnParamUpdateInd(LS_CONNECTION_PARAM_UPDATE_IND_T *p_event_data);

	/* GATT_ACCESS_IND signal handler */
	static void handleSignalGattAccessInd(GATT_ACCESS_IND_T *p_event_data);

	/* LM_EV_DISCONNECT_COMPLETE signal handler */
	static void handleSignalLmDisconnectComplete(HCI_EV_DATA_DISCONNECT_COMPLETE_T *p_event_data);


	/*============================================================================*
	 *  Private Function Implementations
	 *============================================================================*/


	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      appDataInit
	 *л
	 *  DESCRIPTION
	 *      This function is called to initialise the application data structure.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void appDataInit(void)
	{
	    /* Initialise general application timer */
	    if (g_app_data.app_tid != TIMER_INVALID)
	    {
	        TimerDelete(g_app_data.app_tid);
	        g_app_data.app_tid = TIMER_INVALID;
	    }

	    /* Initialise the connection parameter update timer */
	    if (g_app_data.con_param_update_tid != TIMER_INVALID)
	    {
	        TimerDelete(g_app_data.con_param_update_tid);
	        g_app_data.con_param_update_tid = TIMER_INVALID;
	    }

	    /* Initialise the connected client ID */
	    g_app_data.st_ucid = GATT_INVALID_UCID;

	    /* Initialise white list flag */
	    g_app_data.enable_white_list = FALSE;

	    /* Reset the connection parameter variables */
	    g_app_data.conn_interval = 0;
	    g_app_data.conn_latency = 0;
	    g_app_data.conn_timeout = 0;

	    /* Initialise the application GATT data */
	    InitGattData();
	    
	    /* Reset application hardware data */
	    HwDataReset();

	    /* Initialise GAP data structure */
	    GapDataInit();

	    /* Call the required service data initialisation APIs from here */
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      readPersistentStore
	 *
	 *  DESCRIPTION
	 *      This function is used to initialise and read NVM data.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Nothingk
	 *----------------------------------------------------------------------------*/
	static void readPersistentStore(void)
	{
	    /* NVM offset for supported services */
	    uint16 nvm_offset = NVM_MAX_APP_MEMORY_WORDS;
	    uint16 nvm_sanity = 0xffff;
	    bool nvm_start_fresh = FALSE;

	    /* Read persistent storage to find if the device was last bonded to another
	     * device. If the device was bonded, trigger fast undirected advertisements
	     * by setting the white list for bonded host. If the device was not bonded,
	     * trigger undirected advertisements for any host to connect.
	     */
	    
	    Nvm_Read(&nvm_sanity, 
	             sizeof(nvm_sanity), 
	             NVM_OFFSET_SANITY_WORD);

	    if(nvm_sanity == NVM_SANITY_MAGIC)
	    {

	        /* Read Bonded Flag from NVM */
	        Nvm_Read((uint16*)&g_app_data.bonded,
	                  sizeof(g_app_data.bonded),
	                  NVM_OFFSET_BONDED_FLAG);

	        if(g_app_data.bonded)
	        {
	            /* Bonded Host Typed BD Address will only be stored if bonded flag
	             * is set to TRUE. Read last bonded device address.
	             */
	            Nvm_Read((uint16*)&g_app_data.bonded_bd_addr, 
	                       sizeof(TYPED_BD_ADDR_T),
	                       NVM_OFFSET_BONDED_ADDR);

	            /* If device is bonded and bonded address is resolvable then read 
	             * the bonded device's IRK
	             */
	            if(GattIsAddressResolvableRandom(&g_app_data.bonded_bd_addr))
	            {
	                Nvm_Read(g_app_data.irk, 
	                         MAX_WORDS_IRK,
	                         NVM_OFFSET_SM_IRK);
	            }

	        }
	        else /* Case when we have only written NVM_SANITY_MAGIC to NVM but 
	              * didn't get bonded to any host in the last powered session
	              */
	        {
	            /* Any initialisation can be done here for non-bonded devices */
	            
	        }

	        /* Read the diversifier associated with the presently bonded/last 
	         * bonded device.
	         */
	        Nvm_Read(&g_app_data.diversifier, 
	                 sizeof(g_app_data.diversifier),
	                 NVM_OFFSET_SM_DIV);

	        /* If NVM in use, read device name and length from NVM */
	        GapReadDataFromNVM(&nvm_offset);

	    }
	    else /* NVM Sanity check failed means either the device is being brought up 
	          * for the first time or memory has got corrupted in which case 
	          * discard the data and start fresh.
	          */
	    {

	        nvm_start_fresh = TRUE;

	        nvm_sanity = NVM_SANITY_MAGIC;

	        /* Write NVM Sanity word to the NVM */
	        Nvm_Write(&nvm_sanity, 
	                  sizeof(nvm_sanity), 
	                  NVM_OFFSET_SANITY_WORD);

	        /* The device will not be bonded as it is coming up for the first 
	         * time 
	         */
	        g_app_data.bonded = FALSE;

	        /* Write bonded status to NVM */
	        Nvm_Write((uint16*)&g_app_data.bonded, 
	                   sizeof(g_app_data.bonded), 
	                  NVM_OFFSET_BONDED_FLAG);

	        /* When the application is coming up for the first time after flashing 
	         * the image to it, it will not have bonded to any device. So, no LTK 
	         * will be associated with it. Hence, set the diversifier to 0.
	         */
	        g_app_data.diversifier = 0;

	        /* Write the same to NVM. */
	        Nvm_Write(&g_app_data.diversifier, 
	                  sizeof(g_app_data.diversifier),
	                  NVM_OFFSET_SM_DIV);

	        /* If fresh NVM, write device name and length to NVM for the 
	         * first time.
	         */
	        GapInitWriteDataToNVM(&nvm_offset);

	    }

	    /* Read Battery service data from NVM if the devices are bonded and  
	     * update the offset with the number of words of NVM required by 
	     * this service
	     */
	    //Danil
	    //BatteryReadDataFromNVM(&nvm_offset);
	    
	    /* Add the 'read Service data from NVM' API call here, to initialise the
	     * service data, if the device is already bonded. One must take care of the
	     * offset being used for storing the data.
	     */

	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      enableWhiteList
	 *
	 *  DESCRIPTION
	 *      This function enables white list based advertising.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void enableWhiteList(void)
	{
	    if(IsDeviceBonded())
	    {
	        if(!GattIsAddressResolvableRandom(&g_app_data.bonded_bd_addr))
	        {
	            /* Enable white list if the device is bonded and the bonded host 
	             * is not using resolvable random address.
	             */
	            g_app_data.enable_white_list = TRUE;
	        }
	    }
	}


	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      appStartConnUpdateTimer
	 *
	 *  DESCRIPTION
	 *      This function starts the connection parameter update timer.
	 *      g_app_data.con_param_update_tid must be TIMER_INVALID on entry to this
	 *      function.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void appStartConnUpdateTimer(void)
	{
	    if(g_app_data.conn_interval < PREFERRED_MIN_CON_INTERVAL ||
	       g_app_data.conn_interval > PREFERRED_MAX_CON_INTERVAL
	    #if PREFERRED_SLAVE_LATENCY
	           || g_app_data.conn_latency < PREFERRED_SLAVE_LATENCY
	    #endif
	          )
	        {
	            /* Set the number of connection parameter update attempts to zero */
	            g_app_data.num_conn_update_req = 0;

	            /* Start timer to trigger connection parameter update procedure */
	            g_app_data.con_param_update_tid = TimerCreate(
	                                GAP_CONN_PARAM_TIMEOUT,
	                                TRUE, requestConnParamUpdate);

	        }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      requestConnParamUpdate
	 *
	 *  DESCRIPTION
	 *      This function is used to send L2CAP_CONNECTION_PARAMETER_UPDATE_REQUEST 
	 *      to the remote device.
	 *
	 *  PARAMETERS
	 *      tid [in]                ID of timer that has expired
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void requestConnParamUpdate(timer_id tid)
	{
	    /* Application specific preferred parameters */
	     ble_con_params app_pref_conn_param = 
	                {
	                    PREFERRED_MIN_CON_INTERVAL,
	                    PREFERRED_MAX_CON_INTERVAL,
	                    PREFERRED_SLAVE_LATENCY,
	                    PREFERRED_SUPERVISION_TIMEOUT
	                };

	    if(g_app_data.con_param_update_tid == tid)
	    {
	        /* Timer has just expired, so mark it as being invalid */
	        g_app_data.con_param_update_tid = TIMER_INVALID;

	        /* Handle signal as per current state */
	        switch(g_app_data.state)
	        {

	            case app_state_connected:
	            {
	                /* Send Connection Parameter Update request using application 
	                 * specific preferred connection parameters
	                 */

	                if(LsConnectionParamUpdateReq(&g_app_data.con_bd_addr, 
	                                &app_pref_conn_param) != ls_err_none)
	                {
	                    ReportPanic(app_panic_con_param_update);
	                }

	                /* Increment the count for connection parameter update 
	                 * requests 
	                 */
	                ++ g_app_data.num_conn_update_req;

	            }
	            break;

	            default:
	                /* Ignore in other states */
	            break;
	        }

	    } /* Else ignore the timer */
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      appExitAdvertising
	 *
	 *  DESCRIPTION
	 *      This function is called while exiting app_state_fast_advertising and
	 *      app_state_slow_advertising states.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void appExitAdvertising(void)
	{
	    /* Cancel advertisement timer. Must be valid because timer is active
	     * during app_state_fast_advertising and app_state_slow_advertising states.
	     */
	    TimerDelete(g_app_data.app_tid);
	    g_app_data.app_tid = TIMER_INVALID;
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      appAdvertTimerHandler
	 *
	 *  DESCRIPTION
	 *      This function is used to handle Advertisement timer expiry.
	 *
	 *  PARAMETERS
	 *      tid [in]                ID of timer that has expired
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void appAdvertTimerHandler(timer_id tid)
	{
	    /* Based upon the timer id, stop on-going advertisements */
	    if(g_app_data.app_tid == tid)
	    {
	        /* Timer has just expired so mark it as invalid */
	        g_app_data.app_tid = TIMER_INVALID;

	        GattStopAdverts();
	    }/* Else ignore timer expiry, could be because of 
	      * some race condition */
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      appInitExit
	 *
	 *  DESCRIPTION
	 *      This function is called upon exiting from app_state_init state. The 
	 *      application starts advertising after exiting this state.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void appInitExit (void)
	{
	    if(g_app_data.bonded && 
	       !GattIsAddressResolvableRandom(&g_app_data.bonded_bd_addr))
	    {
	        /* If the device is bonded and the bonded device address is not
	         * resolvable random, configure the white list with the bonded 
	         * host address.
	         */
	        if(LsAddWhiteListDevice(&g_app_data.bonded_bd_addr) != ls_err_none)
	        {
	            ReportPanic(app_panic_add_whitelist);
	        }
	    }
	}


	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      handleSignalGattAddDbCfm
	 *
	 *  DESCRIPTION
	 *      This function handles the signal GATT_ADD_DB_CFM.
	 *
	 *  PARAMETERS
	 *      p_event_data [in]       Data supplied by GATT_ADD_DB_CFM signal
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalGattAddDbCfm(GATT_ADD_DB_CFM_T *p_event_data)
	{
	    /* Handle signal as per current state */
	    switch(g_app_data.state)
	    {
	        case app_state_init:
	        {
	            if(p_event_data->result == sys_status_success)
	            {
	                /* Start advertising. */
	                //Danil
	                SetState(app_state_fast_advertising);
	            }
	            else
	            {
	                /* This should never happen */
	                ReportPanic(app_panic_db_registration);
	            }
	        }
	        break;

	        default:
	            /* Control should never come here */
	            ReportPanic(app_panic_invalid_state);
	        break;
	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      handleSignalLmEvConnectionComplete
	 *
	 *  DESCRIPTION
	 *      This function handles the signal LM_EV_CONNECTION_COMPLETE.
	 *
	 *  PARAMETERS
	 *      p_event_data [in]       Data supplied by LM_EV_CONNECTION_COMPLETE
	 *                              signal
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalLmEvConnectionComplete(
	                                     LM_EV_CONNECTION_COMPLETE_T *p_event_data)
	{
	    /* Store the connection parameters. */
	    g_app_data.conn_interval = p_event_data->data.conn_interval;
	    g_app_data.conn_latency = p_event_data->data.conn_latency;
	    g_app_data.conn_timeout = p_event_data->data.supervision_timeout;
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      handleSignalGattCancelConnectCfm
	 *
	 *  DESCRIPTION
	 *      This function handles the signal GATT_CANCEL_CONNECT_CFM.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalGattCancelConnectCfm(void)
	{
	    /* Handle signal as per current state.
	     *
	     * The application follows this sequence in advertising state:
	     *
	     * 1. Fast advertising for FAST_CONNECTION_ADVERT_TIMEOUT_VALUE seconds:
	     *    If the application is bonded to a remote device then during this
	     *    period it will use the white list for the first
	     *    BONDED_DEVICE_ADVERT_TIMEOUT_VALUE seconds, and for the remaining
	     *    time (FAST_CONNECTION_ADVERT_TIMEOUT_VALUE - 
	     *    BONDED_DEVICE_ADVERT_TIMEOUT_VALUE seconds), it will do fast
	     *    advertising without any white list.
	     *
	     * 2. Slow advertising for SLOW_CONNECTION_ADVERT_TIMEOUT_VALUE seconds
	     */
	    switch(g_app_data.state)
	    {
	        case app_state_fast_advertising:
	        {
	            bool fastConns = TRUE;

	            if(g_app_data.enable_white_list == TRUE)
	            {
	                /* Bonded device advertisements stopped. Reset the white
	                 * list
	                 */
	                if(LsDeleteWhiteListDevice(&g_app_data.bonded_bd_addr)
	                    != ls_err_none)
	                {
	                    ReportPanic(app_panic_delete_whitelist);
	                }

	                /* Case of stopping advertisements for the bonded device 
	                 * at the expiry of BONDED_DEVICE_ADVERT_TIMEOUT_VALUE timer
	                 */
	                g_app_data.enable_white_list = FALSE;

	                fastConns = TRUE;
	            }

	            if(fastConns)
	            {
	                GattStartAdverts(&g_app_data.bonded_bd_addr, TRUE);

	                /* Remain in same state */
	            }
	            else
	            {
	                SetState(app_state_slow_advertising);
	            }
	        }
	        break;

	        case app_state_slow_advertising:
	            /* If the application was doing slow advertisements, stop
	             * advertising and move to idle state.
	             */
	            SetState(app_state_idle);
	        break;
	        
	        default:
	            /* Control should never come here */

	            ReportPanic(app_panic_invalid_state);
	        break;
	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      handleSignalGattConnectCfm
	 *
	 *  DESCRIPTION
	 *      This function handles the signal GATT_CONNECT_CFM.
	 *
	 *  PARAMETERS
	 *      p_event_data [in]       Data supplied by GATT_CONNECT_CFM signal
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalGattConnectCfm(GATT_CONNECT_CFM_T *p_event_data)
	{
	    /* Handle signal as per current state */
	    switch(g_app_data.state)
	    {
	        case app_state_fast_advertising:   /* FALLTHROUGH */
	        case app_state_slow_advertising:
	        {
	            if(p_event_data->result == sys_status_success)
	            {
	                /* Store received UCID */
	                g_app_data.st_ucid = p_event_data->cid;

	                /* Store connected BD Address */
	                g_app_data.con_bd_addr = p_event_data->bd_addr;

	                if(g_app_data.bonded && 
	                   GattIsAddressResolvableRandom(&g_app_data.bonded_bd_addr) &&
	                   (SMPrivacyMatchAddress(&p_event_data->bd_addr,
	                                          g_app_data.irk,
	                                          MAX_NUMBER_IRK_STORED, 
	                                          MAX_WORDS_IRK) < 0))
	                {
	                    /* Application was bonded to a remote device using 
	                     * resolvable random address and application has failed 
	                     * to resolve the remote device address to which we just 
	                     * connected, so disconnect and start advertising again
	                     */
	                    SetState(app_state_disconnecting);
	                }
	                else
	                {
	                    /* Enter connected state 
	                     * - If the device is not bonded OR
	                     * - If the device is bonded and the connected host doesn't 
	                     *   support Resolvable Random address OR
	                     * - If the device is bonded and connected host supports 
	                     *   Resolvable Random address and the address gets resolved
	                     *   using the stored IRK key
	                     */
	                    SetState(app_state_connected);

	                }
	            }
	            else
	            {
	                /* Connection failure - Trigger fast advertisements */
	                if(g_app_data.state == app_state_slow_advertising)
	                {
	                    SetState(app_state_fast_advertising);
	                }
	                else
	                {
	                    /* Already in app_state_fast_advertising state, so just 
	                     * trigger fast advertisements
	                     */
	                    GattStartAdverts(&g_app_data.bonded_bd_addr, TRUE);
	                }
	            }
	        }
	        break;

	        default:
	            /* Control should never come here */
	            ReportPanic(app_panic_invalid_state);
	        break;
	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      handleSignalSmKeysInd
	 *
	 *  DESCRIPTION
	 *      This function handles the signal SM_KEYS_IND and copies the IRK from it.
	 *
	 *  PARAMETERS
	 *      p_event_data [in]       Data supplied by SM_KEYS_IND signal
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalSmKeysInd(SM_KEYS_IND_T *p_event_data)
	{
	    /* Handle signal as per current state */
	    switch(g_app_data.state)
	    {
	        case app_state_connected:
	        {

	            /* Store the diversifier which will be used for accepting/rejecting
	             * the encryption requests.
	             */
	            g_app_data.diversifier = (p_event_data->keys)->div;

	            /* Write the new diversifier to NVM */
	            Nvm_Write(&g_app_data.diversifier,
	                      sizeof(g_app_data.diversifier), 
	                      NVM_OFFSET_SM_DIV);

	            /* Store IRK if the connected host is using random resolvable 
	             * address. IRK is used afterwards to validate the identity of 
	             * connected host 
	             */
	            if(GattIsAddressResolvableRandom(&g_app_data.con_bd_addr)) 
	            {
	                MemCopy(g_app_data.irk, 
	                        (p_event_data->keys)->irk,
	                        MAX_WORDS_IRK);

	                /* If bonded device address is resolvable random
	                 * then store IRK to NVM 
	                 */
	                Nvm_Write(g_app_data.irk, 
	                          MAX_WORDS_IRK, 
	                          NVM_OFFSET_SM_IRK);
	            }
	        }
	        break;

	        default:
	            /* Control should never come here */
	            ReportPanic(app_panic_invalid_state);
	        break;
	    }
	}


	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      handleSignalSmSimplePairingCompleteInd
	 *
	 *  DESCRIPTION
	 *      This function handles the signal SM_SIMPLE_PAIRING_COMPLETE_IND.
	 *
	 *  PARAMETERS
	 *      p_event_data [in]       Data supplied by SM_SIMPLE_PAIRING_COMPLETE_IND
	 *                              signal
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalSmSimplePairingCompleteInd(
	                                 SM_SIMPLE_PAIRING_COMPLETE_IND_T *p_event_data)
	{

	    /* Handle signal as per current state */
	    switch(g_app_data.state)
	    {
	        case app_state_connected:
	        {
	            if(p_event_data->status == sys_status_success)
	            {
	                /* Store bonded host information to NVM. This includes
	                 * application and service specific information.
	                 */
	                g_app_data.bonded = TRUE;
	                g_app_data.bonded_bd_addr = p_event_data->bd_addr;

	                /* Store bonded host typed bd address to NVM */

	                /* Write one word bonded flag */
	                Nvm_Write((uint16*)&g_app_data.bonded, 
	                          sizeof(g_app_data.bonded), 
	                          NVM_OFFSET_BONDED_FLAG);

	                /* Write typed bd address of bonded host */
	                Nvm_Write((uint16*)&g_app_data.bonded_bd_addr, 
	                          sizeof(TYPED_BD_ADDR_T), 
	                          NVM_OFFSET_BONDED_ADDR);

	                /* Configure white list with the Bonded host address only 
	                 * if the connected host doesn't support random resolvable
	                 * addresses
	                 */
	                if(!GattIsAddressResolvableRandom(&g_app_data.bonded_bd_addr))
	                {
	                    /* It is important to note that this application does not
	                     * support Reconnection Address. In future, if the
	                     * application is enhanced to support Reconnection Address,
	                     * make sure that we don't add Reconnection Address to the
	                     * white list
	                     */
	                    if(LsAddWhiteListDevice(&g_app_data.bonded_bd_addr) !=
	                        ls_err_none)
	                    {
	                        ReportPanic(app_panic_add_whitelist);
	                    }

	                }

	                /* If the devices are bonded then send notification to all 
	                 * registered services for the same so that they can store
	                 * required data to NVM.
	                 */
	                //BatteryBondingNotify();
	                
	                /* Add the Service Bonding Notify API here */
	            }
	            else
	            {
	                /* If application is already bonded to this host and pairing 
	                 * fails, remove device from the white list.
	                 */
	                if(g_app_data.bonded)
	                {
	                    if(LsDeleteWhiteListDevice(&g_app_data.bonded_bd_addr) != 
	                                        ls_err_none)
	                    {
	                        ReportPanic(app_panic_delete_whitelist);
	                    }

	                    g_app_data.bonded = FALSE;
	                }

	                /* The case when pairing has failed. The connection may still be
	                 * there if the remote device hasn't disconnected it. The
	                 * remote device may retry pairing after a time defined by its
	                 * own application. So reset all other variables except the
	                 * connection specific ones.
	                 */

	                /* Update bonded status to NVM */
	                Nvm_Write((uint16*)&g_app_data.bonded,
	                          sizeof(g_app_data.bonded),
	                          NVM_OFFSET_BONDED_FLAG);

	                /* Initialise the data of used services as the device is no 
	                 * longer bonded to the remote host.
	                 */
	                GapDataInit();
	                //BatteryDataInit();
	                
	                /* Call all the APIs which initailise the required service(s)
	                 * supported by the application.
	                 */

	            }
	        }
	        break;

	        default:
	            /* Firmware may send this signal after disconnection. So don't 
	             * panic but ignore this signal.
	             */
	        break;
	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAMEk
	 *      handleSignalSmDivApproveInd
	 *
	 *  DESCRIPTION
	 *      This function handles the signal SM_DIV_APPROVE_IND.
	 *
	 *  PARAMETERS
	 *      p_event_data [in]       Data supplied by SM_DIV_APPROVE_IND signal
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalSmDivApproveInd(SM_DIV_APPROVE_IND_T *p_event_data)
	{
	    /* Handle signal as per current state */
	    switch(g_app_data.state)
	    {
	        
	        /* Request for approval from application comes only when pairing is not
	         * in progress
	         */
	        case app_state_connected:
	        {
	            sm_div_verdict approve_div = SM_DIV_REVOKED;
	            
	            /* Check whether the application is still bonded (bonded flag gets
	             * reset upon 'connect' button press by the user). Then check 
	             * whether the diversifier is the same as the one stored by the 
	             * application
	             */
	            if(g_app_data.bonded)
	            {
	                if(g_app_data.diversifier == p_event_data->div)
	                {
	                    approve_div = SM_DIV_APPROVED;
	                }
	            }

	            SMDivApproval(p_event_data->cid, approve_div);
	        }
	        break;

	        default:
	            /* Control should never come here */
	            ReportPanic(app_panic_invalid_state);
	        break;

	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      handleSignalLsConnParamUpdateCfm
	 *
	 *  DESCRIPTION
	 *      This function handles the signal LS_CONNECTION_PARAM_UPDATE_CFM.
	 *
	 *  PARAMETERS
	 *      p_event_data [in]       Data supplied by LS_CONNECTION_PARAM_UPDATE_CFM
	 *                              signal
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalLsConnParamUpdateCfm(
	                            LS_CONNECTION_PARAM_UPDATE_CFM_T *p_event_data)
	{
	    /* Handle signal as per current state */
	    switch(g_app_data.state)
	    {
	        case app_state_connected:
	        {
	            /* Received in response to the L2CAP_CONNECTION_PARAMETER_UPDATE 
	             * request sent from the slave after encryption is enabled. If 
	             * the request has failed, the device should send the same request
	             * again only after Tgap(conn_param_timeout). Refer Bluetooth 4.0
	             * spec Vol 3 Part C, Section 9.3.9 and profile spec.
	             */
	            if ((p_event_data->status != ls_err_none) &&
	                    (g_app_data.num_conn_update_req < 
	                    MAX_NUM_CONN_PARAM_UPDATE_REQS))
	            {
	                /* Delete timer if running */
	                if (g_app_data.con_param_update_tid != TIMER_INVALID)
	                {
	                    TimerDelete(g_app_data.con_param_update_tid);
	                }

	                g_app_data.con_param_update_tid = TimerCreate(
	                                             GAP_CONN_PARAM_TIMEOUT,
	                                             TRUE, requestConnParamUpdate);
	            }
	        }
	        break;

	        default:
	            /* Control should never come here */
	            ReportPanic(app_panic_invalid_state);
	        break;
	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      handleSignalLsConnParamUpdateInd
	 *
	 *  DESCRIPTION
	 *      This function handles the signal LS_CONNECTION_PARAM_UPDATE_IND.
	 *
	 *  PARAMETERS
	 *      p_event_data [in]       Data supplied by LS_CONNECTION_PARAM_UPDATE_IND
	 *                              signal
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalLsConnParamUpdateInd(
	                                 LS_CONNECTION_PARAM_UPDATE_IND_T *p_event_data)
	{

	    /* Handle signal as per current state */
	    switch(g_app_data.state)
	    {

	        case app_state_connected:
	        {
	            /* Delete timer if running */
	            if (g_app_data.con_param_update_tid != TIMER_INVALID)
	            {
	                TimerDelete(g_app_data.con_param_update_tid);
	                g_app_data.con_param_update_tid = TIMER_INVALID;
	            }

	            /* Store the new connection parameters. */
	            g_app_data.conn_interval = p_event_data->conn_interval;
	            g_app_data.conn_latency = p_event_data->conn_latency;
	            g_app_data.conn_timeout = p_event_data->supervision_timeout;
	            
	            /* Connection parameters have been updated. Check if new parameters 
	             * comply with application preferred parameters. If not, application
	             * shall trigger Connection parameter update procedure.
	             */
	            appStartConnUpdateTimer();
	        }
	        break;

	        default:
	            /* Control should never come here */
	            ReportPanic(app_panic_invalid_state);
	        break;
	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      handleSignalGattAccessInd
	 *
	 *  DESCRIPTION
	 *      This function handles GATT_ACCESS_IND messages for attributes maintained
	 *      by the application.
	 *
	 *  PARAMETERS
	 *      p_event_data [in]       Data supplied by GATT_ACCESS_IND message
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalGattAccessInd(GATT_ACCESS_IND_T *p_event_data)
	{
	    /*DebugIfWriteString("| handleSignalGattAccessInd");
	    DebugIfWriteString(" | p_event_data->flags = "); DebugIfWriteUint16(p_event_data->flags);
	    DebugIfWriteString(" | p_event_data->handle = "); DebugIfWriteUint16(p_event_data->handle);
	    DebugIfWriteString(" | p_event_data->cid = "); DebugIfWriteUint16(p_event_data->cid);
	    DebugIfWriteString(" | p_event_data->offset = "); DebugIfWriteUint16(p_event_data->offset);
	    DebugIfWriteString(" | p_event_data->size_value = "); DebugIfWriteUint16(p_event_data->size_value);
	    */

	    /* Handle signal as per current state */
	    switch(g_app_data.state)
	    {
	        case app_state_connected:
	        {
	            /* Received GATT ACCESS IND with write access */
	            if(p_event_data->flags == 
	                (ATT_ACCESS_WRITE | 
	                 ATT_ACCESS_PERMISSION | 
	                 ATT_ACCESS_WRITE_COMPLETE))
	            {
	                HandleAccessWrite(p_event_data);
	            }
	            /* Received GATT ACCESS IND with read access */
	            else if(p_event_data->flags == 
	                (ATT_ACCESS_READ | 
	                ATT_ACCESS_PERMISSION))
	            {
	                HandleAccessRead(p_event_data);
	            }
	            else
	            {
	                /* No other request is supported */
	                GattAccessRsp(p_event_data->cid, p_event_data->handle, 
	                              gatt_status_request_not_supported,
	                              0, NULL);
	            }
	        }
	        break;

	        default:
	            /* Control should never come here */
	            ReportPanic(app_panic_invalid_state);
	        break;
	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      handleSignalLmDisconnectComplete
	 *
	 *  DESCRIPTION
	 *      This function handles LM Disconnect Complete event which is received
	 *      at the completion of disconnect procedure triggered either by the 
	 *      device or remote host or because of link loss.
	 *
	 *  PARAMETERS
	 *      p_event_data [in]       Data supplied by LM_EV_DISCONNECT_COMPLETE
	 *                              signal
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	static void handleSignalLmDisconnectComplete(
	                HCI_EV_DATA_DISCONNECT_COMPLETE_T *p_event_data)
	{

	    /* Set UCID to INVALID_UCID */
	    g_app_data.st_ucid = GATT_INVALID_UCID;

	    /* Reset the connection parameter variables. */
	    g_app_data.conn_interval = 0;
	    g_app_data.conn_latency = 0;
	    g_app_data.conn_timeout = 0;
	    
	    /* LM_EV_DISCONNECT_COMPLETE event can have following disconnect reasons:
	     *
	     * HCI_ERROR_CONN_TIMEOUT - Link Loss case
	     * HCI_ERROR_CONN_TERM_LOCAL_HOST - Disconnect triggered by device
	     * HCI_ERROR_OETC_* - Other end (i.e., remote host) terminated connection
	     */
	    /* Handle signal as per current state */
	    switch(g_app_data.state)
	    {
	        case app_state_connected:
	            /* Initialise Application data instance */
	            appDataInit();

	            /* FALLTHROUGH */

	        case app_state_disconnecting:
	        {

	            /* Link Loss Case */
	            if(p_event_data->reason == HCI_ERROR_CONN_TIMEOUT)
	            {
	                /* Start undirected advertisements by moving to 
	                 * app_state_fast_advertising state
	                 */
	                SetState(app_state_fast_advertising);
	            }
	            else if(p_event_data->reason == HCI_ERROR_CONN_TERM_LOCAL_HOST)
	            {

	                if(g_app_data.state == app_state_connected)
	                {
	                    /* It is possible to receive LM_EV_DISCONNECT_COMPLETE 
	                     * event in app_state_connected state at the expiry of 
	                     * lower layers' ATT/SMP timer leading to disconnect
	                     */

	                    /* Start undirected advertisements by moving to 
	                     * app_state_fast_advertising state
	                     */
	                    SetState( app_state_fast_advertising);
	                }
	                else
	                {
	                    /* Case when application has triggered disconnect */

	                    if(g_app_data.bonded)
	                    {
	                        /* If the device is bonded and host uses resolvable 
	                         * random address, the device initiates disconnect 
	                         * procedure if it gets reconnected to a different 
	                         * host, in which case device should trigger fast 
	                         * advertisements after disconnecting from the last 
	                         * connected host.
	                         */
	                        if(GattIsAddressResolvableRandom(
	                                            &g_app_data.bonded_bd_addr) &&
	                           (SMPrivacyMatchAddress(&g_app_data.con_bd_addr,
	                                            g_app_data.irk,
	                                            MAX_NUMBER_IRK_STORED, 
	                                            MAX_WORDS_IRK) < 0))
	                        {
	                            SetState( app_state_fast_advertising);
	                        }
	                        else
	                        {
	                            /* Else move to app_state_idle state because of 
	                             * user action or inactivity
	                             */
	                            SetState(app_state_idle);
	                        }
	                    }
	                    else /* Case of Bonding/Pairing removal */
	                    {
	                        /* Start undirected advertisements by moving to 
	                         * app_state_fast_advertising state
	                         */
	                        SetState(app_state_fast_advertising);
	                    }
	                }

	            }
	            else /* Remote user terminated connection case */
	            {
	                /* If the device has not bonded but disconnected, it may just 
	                 * have discovered the services supported by the application or 
	                 * read some un-protected characteristic value like device name 
	                 * and disconnected. The application should be connectable 
	                 * because the same remote device may want to reconnect and 
	                 * bond. If not the application should be discoverable by other 
	                 * devices.
	                 */
	                if(!g_app_data.bonded)
	                {
	                    SetState( app_state_fast_advertising);
	                }
	                else /* Case when disconnect is triggered by a bonded Host */
	                {
	                    SetState( app_state_idle);
	                }
	            }
	        }
	        break;
	        
	        default:
	            /* Control should never come here */
	            ReportPanic(app_panic_invalid_state);
	        break;
	    }
	}

	/*============================================================================*
	 *  Public Function Implementations
	 *============================================================================*/

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      ReportPanic
	 *
	 *  DESCRIPTION
	 *      This function calls firmware panic routine and gives a single point 
	 *      of debugging any application level panics.
	 *
	 *  PARAMETERS
	 *      panic_code [in]         Code to supply to firmware Panic function.
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	extern void ReportPanic(app_panic_code panic_code)
	{
	    /* Raise panic */
	    Panic(panic_code);
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      HandleShortButtonPress
	 *
	 *  DESCRIPTION
	 *      This function contains handling of short button press. If connected,
	 *      the device disconnects from the connected host else it triggers
	 *      advertisements.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	extern void HandleShortButtonPress(void)
	{


	    /* Handle signal as per current state */
	    switch(g_app_data.state)
	    {
	        case app_state_connected:
	            /* Disconnect from the connected host */
	            SetState(app_state_disconnecting);
	            
	            /* As per the specification Vendor may choose to initiate the 
	             * idle timer which will eventually initiate the disconnect.
	             */
	             
	        break;

	        case app_state_idle:
	            /* Trigger fast advertisements */
	            SetState(app_state_fast_advertising);
	        break;

	        default:
	            /* Ignore in remaining states */
	        break;

	    }

	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      SetState
	 *
	 *  DESCRIPTION
	 *      This function is used to set the state of the application.
	 *
	 *  PARAMETERS
	 *      new_state [in]          State to move to
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	extern void SetState(app_state new_state)
	{
	    /* Check that the new state is not the same as the current state */
	    app_state old_state = g_app_data.state;
	    
	    if (old_state != new_state)
	    {
	        /* Exit current state */
	        switch (old_state)
	        {
	            case app_state_init:
	                appInitExit();
	            break;

	            case app_state_disconnecting:
	                /* Common things to do whenever application exits
	                 * app_state_disconnecting state.
	                 */

	                /* Initialise application and used services data structure 
	                 * while exiting Disconnecting state
	                 */
	                appDataInit();
	            break;

	            case app_state_fast_advertising:  /* FALLTHROUGH */
	            case app_state_slow_advertising:
	                /* Common things to do whenever application exits
	                 * APP_*_ADVERTISING state.
	                 */
	                appExitAdvertising();
	            break;

	            case app_state_connected:
	                /* The application may need to maintain the values of some
	                 * profile specific data across connections and power cycles.
	                 * These values would have changed in 'connected' state. So,
	                 * update the values of this data stored in the NVM.
	                 */
	            break;

	            case app_state_idle:
	                /* Nothing to do */
	            break;

	            default:
	                /* Nothing to do */
	            break;
	        }

	        /* Set new state */
	        g_app_data.state = new_state;

	        /* Enter new state */
	        switch (new_state)
	        {
	            case app_state_fast_advertising:
	            {
	                /* Enable white list if application is bonded to some remote 
	                 * device and that device is not using resolvable random 
	                 * address.
	                 */
	                enableWhiteList();
	                /* Trigger fast advertisements. */
	                GattTriggerFastAdverts(&g_app_data.bonded_bd_addr);

	            }
	            break;

	            case app_state_slow_advertising:
	                /* Start slow advertisements */
	                GattStartAdverts(&g_app_data.bonded_bd_addr, FALSE);
	            break;

	            case app_state_idle:
	            break;

	            case app_state_connected:
	            {
	                /* Common things to do whenever application enters
	                 * app_state_connected state.
	                 */
	             }
	            break;

	            case app_state_disconnecting:
	                /* Disconnect the link */
	                GattDisconnectReq(g_app_data.st_ucid);
	            break;

	            default:
	            break;
	        }
	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      GetState
	 *
	 *  DESCRIPTION
	 *      This function returns the current state of the application.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Current application state
	 *----------------------------------------------------------------------------*/
	extern app_state GetState(void)
	{
	    return g_app_data.state;
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      IsWhiteListEnabled
	 *
	 *  DESCRIPTION
	 *      This function returns whether white list is enabled or not.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      TRUE if white list is enabled, FALSE otherwise.
	 *----------------------------------------------------------------------------*/
	extern bool IsWhiteListEnabled(void)
	{
	    return g_app_data.enable_white_list;
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      HandlePairingRemoval
	 *
	 *  DESCRIPTION
	 *      This function contains pairing removal handling.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	extern void HandlePairingRemoval(void)
	{

	    /* Remove bonding information */

	    /* The device will no longer be bonded */
	    g_app_data.bonded = FALSE;

	    /* Write bonded status to NVM */
	    Nvm_Write((uint16*)&g_app_data.bonded, 
	              sizeof(g_app_data.bonded), 
	              NVM_OFFSET_BONDED_FLAG);


	    switch(g_app_data.state)
	    {

	        case app_state_connected:
	        {
	            /* Disconnect from the connected host before triggering 
	             * advertisements again for any host to connect. Application
	             * and services data related to bonding status will get 
	             * updated while exiting disconnecting state.
	             */
	            SetState(app_state_disconnecting);

	            /* Reset and clear the white list */
	            LsResetWhiteList();
	        }
	        break;

	        case app_state_fast_advertising:
	        case app_state_slow_advertising:
	        {
	            /* Initialise application and services data related to 
	             * bonding status
	             */
	            appDataInit();

	            /* Stop advertisements first as it may be making use of the white 
	             * list. Once advertisements are stopped, reset the white list
	             * and trigger advertisements again for any host to connect.
	             */
	            GattStopAdverts();
	        }
	        break;

	        case app_state_disconnecting:
	        {
	            /* Disconnect procedure is on-going, so just reset the white list 
	             * and wait for the procedure to complete before triggering 
	             * advertisements again for any host to connect. Application
	             * and services data related to bonding status will get 
	             * updated while exiting disconnecting state.
	             */
	            LsResetWhiteList();
	        }
	        break;

	        default: /* app_state_init / app_state_idle handling */
	        {
	            /* Initialise application and services data related to 
	             * bonding status
	             */
	            appDataInit();

	            /* Reset and clear the white list */
	            LsResetWhiteList();

	            /* Start fast undirected advertisements */
	            SetState(app_state_fast_advertising);
	        }
	        break;

	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      StartAdvertTimer
	 *
	 *  DESCRIPTION
	 *      This function starts the advertisement timer.
	 *
	 *  PARAMETERS
	 *      interval [in]           Timer duration, microseconds
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	extern void StartAdvertTimer(uint32 interval)
	{
	    /* Cancel existing timer, if valid */
	    if (g_app_data.app_tid != TIMER_INVALID)
	    {
	        TimerDelete(g_app_data.app_tid);
	    }

	    /* Start advertisement timer  */
	    //Danil
	    g_app_data.app_tid = TimerCreate(interval, TRUE, appAdvertTimerHandler);
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      IsDeviceBonded
	 *
	 *  DESCRIPTION
	 *      This function returns the status whether the connected device is 
	 *      bonded or not.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      TRUE if device is bonded, FALSE if not.
	 *----------------------------------------------------------------------------*/
	extern bool IsDeviceBonded(void)
	{
	    return g_app_data.bonded;
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      GetConnectionID
	 *
	 *  DESCRIPTION
	 *      This function returns the connection identifier.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Connection identifier.
	 *----------------------------------------------------------------------------*/
	extern uint16 GetConnectionID(void)
	{
	    return g_app_data.st_ucid;
	}

	/*============================================================================*
	 *  System Callback Function Implementations
	 *============================================================================*/

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      AppPowerOnReset
	 *
	 *  DESCRIPTION
	 *      This user application function is called just after a power-on reset
	 *      (including after a firmware panic), or after a wakeup from Hibernate or
	 *      Dormant sleep states.
	 *
	 *      At the time this function is called, the last sleep state is not yet
	 *      known.
	 *
	 *      NOTE: this function should only contain code to be executed after a
	 *      power-on reset or panic. Code that should also be executed after an
	 *      HCI_RESET should instead be placed in the AppInit() function.
	 *
	 *  PARAMETERS
	 *      None
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	extern void AppPowerOnReset(void)
	{
	    /* Code that is only executed after a power-on reset or firmware panic
	     * should be implemented here - e.g. configuring application constants
	     */
	}


	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      uartRxDataCallback
	 *
	 *  DESCRIPTION
	 *      This is an internal callback function (of type uart_data_in_fn) that
	 *      will be called by the UART driver when any data is received over UART.
	 *      See DebugInit in the Firmware Library documentation for details.
	 *
	 *  PARAMETERS
	 *      p_rx_buffer [in]   Pointer to the receive buffer (uint8 if 'unpacked'
	 *                         or uint16 if 'packed' depending on the chosen UART
	 *                         data mode - this application uses 'unpacked')
	 *
	 *      length [in]        Number of bytes ('unpacked') or words ('packed')
	 *                         received
	 *
	 *      p_additional_req_data_length [out]
	 *                         Number of additional bytes ('unpacked') or words
	 *                         ('packed') this application wishes to receive
	 *
	 *  RETURNS
	 *      The number of bytes ('unpacked') or words ('packed') that have been
	 *      processed out of the available data.
	 *----------------------------------------------------------------------------*/
	uint8 data_char[2];
	static uint16 uartRxDataCallback(void   *p_rx_buffer,
	                                 uint16  length,
	                                 uint16 *p_additional_req_data_length)
	{
	    if (length == 1)
	    {
	        
	        MemCopy(data_char, p_rx_buffer, 1);
	        //ArrayPrint(p_rx_buffer, 1);

	        if ( data_char[0] == '{') Request_Read_Flag++;

	        if ( Request_Buf_Pointer >= REQUEST_BUF_SIZE ) 
	        {
	            Request_Read_Flag = 0;
	            ArrayClear(Request_Buf, REQUEST_BUF_SIZE);
	            Request_Buf_Pointer=0;
	        }

	        if ( Request_Read_Flag )
	        {   
	            //Danil
	            
	            Request_Buf[Request_Buf_Pointer++] = data_char[0];

	            if( data_char[0] == '}') Request_Read_Flag--;

	            if (Request_Read_Flag == 0)
	            {
	                ArrayClear(Responce_Buf, Responce_Buf_Lenght);
	                Responce_Buf_Lenght=0;
	               
	                //Danil
	                /*DebugWriteString("CSR U: ");
	                ArrayPrint(Request_Buf, Request_Buf_Pointer);
	                DebugWriteString("\r\n");*/           
	                
	                
	                UsartDataParce(Request_Buf, Request_Buf_Pointer, Responce_Buf, &Responce_Buf_Lenght);

	                /*DebugWriteString("CSR U: ");
	                ArrayPrint(Responce_Buf, Responce_Buf_Lenght);
	                DebugWriteString("\r\n");*/

	                ArrayClear(Request_Buf, REQUEST_BUF_SIZE);
				    Request_Buf_Pointer = 0;
				    Request_Read_Flag = 0; //Danil Last
				    Request_Read_Attempts = 0; //Danil Last

				    ArrayClear(Responce_Buf, RESPONCE_BUF_SIZE);
				    Responce_Buf_Lenght = 0;
	            }
	        }

	        data_char[0] = 0;
	    }

	    *p_additional_req_data_length = 1;
	    
	    /* Return the number of bytes that have been processed */
	    return length;
	}

	extern void ArrayClear(uint8* data, uint16 data_lenght)
	{
	    uint16 pointer;
	    for (pointer =0; pointer<data_lenght; pointer++)
	        data[pointer]=0;
	}

	extern void ArrayPrint(uint8* data, uint16 data_lenght)
	{
	    uint16 pointer;
	    for (pointer =0; pointer<data_lenght; pointer++)
	        DebugWriteChar(data[pointer]);
	}

	extern void ArrayPut(uint8* array, uint16 *array_start, uint8* data, uint16 data_lenght)
	{
	    uint16 pointer;
	    for (pointer = 0; pointer<data_lenght; pointer++) {
	        array[*array_start + pointer] = data[pointer];
	    }
	    *array_start += data_lenght;
	}

	extern void ArrayPutArray(uint8* array, uint16 array_lenght, uint8* data, uint16 data_lenght, uint16 array_start, uint16 data_start, uint16 put_lenght)
	{
	    uint16 write_lenght = put_lenght;

	    if (array_lenght < array_start) return;
	    if (data_lenght < data_start)   return; 

	    if( (array_lenght-array_start) < put_lenght ) write_lenght = array_lenght-array_start;
	    if ( ((write_lenght < put_lenght)&&(write_lenght > (data_lenght-data_start))) || ((data_lenght-data_start)<put_lenght) )
	        write_lenght = data_lenght-data_start;
	        
	    uint16 pointer;    
	    for (pointer = 0; pointer<write_lenght; pointer++) {
	        array[array_start + pointer] = data[data_start + pointer];
	    }
	}

	extern uint16 StrToInt(uint8* data, uint16 data_lenght)
	{
	   if (data_lenght > DATA_UINT_LEN) return 0;
	    
	    unsigned int digit=1;
	    unsigned int result=0;

	    unsigned int i=data_lenght;
	    while (i--) {
	        if (data[i]>47 && data[i]<58) {
	            result += (data[i]-48)*digit;
	            digit*=10;
	        }
	    }
	    
	    return result;
	}

	extern void IntToStr(uint16 num, uint8 *data, uint8 *data_lenght)
	{
	    uint8 array[DATA_UINT_LEN];
	    uint16 a = 1;
	    uint8 len = 0;  
	    uint8 dig = 0;
	    
	    if (num==0) {
	        len = 1;
	        data[0] = 48;
	    } else {
	        
	        
	        while(num + 1 > a){
	            len++; a*=10;
	        }

	        
	        if (len==5) 
	            a=10000; 
	        else 
	            a/=10;
	        
	        for(dig = 0; dig<len; dig++ ){
	            data[dig] = 48+num/a%10;
	            a/=10; 
	        }
	    }
	    
	    *data_lenght = len;
	}

#endif















extern uint8 UsartDataParce(uint8* request, uint16 request_lenght, uint8* responce, uint16 *responce_lenght)
{
    //Danil
    /*DebugIfWriteString("UsartDataParce\r\n");
    DebugWriteString("UsartDataParce: ");
    ArrayPrint(request, request_lenght);
    DebugWriteString("\r\n");*/
    //uint16 gatt_db_length = 0;
    //uint16 *p_gatt_db = NULL;   
    
    
    uint8 secure_code = 4;

    BleDataLenght = 0;
    if ( FindParameter(request, request_lenght, secure_code, BleObj, BLE_OBJ_LEN, BLE_OBJ_SECURE, BleData, &BleDataLenght) == 0) 
    {
        if ( FindParameter(request, request_lenght, secure_code, Name, NAME_LEN, NAME_SECURE, NameData, &NameDataLenght) == 0) 
        {
            updateDeviceName(NameDataLenght, NameData);
            //ArrayPrint(NameData, NameDataLenght); DebugIfWriteString("\r\n");
        }
        


        BleEnableFlag = StrToInt(BleData, BleDataLenght);
        if (BleEnableFlag) {
        	
            if (BleStatusEnableFlag == 0) 
            {
            	DebugIfWriteString("{ble_status: ble_advertased}\r\n");
                BleStatusEnableFlag = 1;               
                GattStartAdverts(&g_app_data.bonded_bd_addr, TRUE);
            }   
            else
            {
            	DebugIfWriteString("{ble_status: ble_advertase}\r\n");
            }        
        } else {
        	
            if (BleStatusEnableFlag != 0) 
            {
            	DebugIfWriteString("{ble_status: ble_hided}\r\n");
                BleStatusEnableFlag = 0; 
                GattDisconnectReq(g_app_data.st_ucid);
                GattStopAdverts();
                        
                //GattCancelConnectReq();
            }
            else
            {
            	DebugIfWriteString("{ble_status: ble_hide}\r\n");
            }

            ArrayClear(Request_Buf, REQUEST_BUF_SIZE);
		    Request_Buf_Pointer = 0;
		    Request_Read_Flag = 0;
		    Request_Read_Attempts = 0;

		    ArrayClear(Responce_Buf, RESPONCE_BUF_SIZE);
		    Responce_Buf_Lenght = 0;

            ArrayClear(Data_Write_Buf, DATA_BUF_SIZE);         
            Data_Write_Buf_Lenght = 0;      
            Data_Write_Buf_Flag = 0;   
            Data_Write_Buf_Chunk_Lenght = 0;  
            Data_Write_Buf_Chunk_Pointer = 0;   
        
            ArrayClear(Data_Read_Buf, DATA_BUF_SIZE);
            Data_Read_Buf_Lenght = 0;  
            Data_Read_Buf_Flag = 0;
            Data_Read_Buf_Chunk_Lenght = 0;  
            Data_Read_Buf_Chunk_Pointer = 0;  

            AmountDataFlag = 0;
	        ArrayClear(AmountData, AMOUNT_DATA_MAX_LEN);
	        AmountDataLenght = 0;
        }

    }
    else
    {
        AmountDataFlag = 0;
        ArrayClear(AmountData, AMOUNT_DATA_MAX_LEN);
        AmountDataLenght = 0;
        
        ArrayClear(Data_Read_Buf, DATA_BUF_SIZE);                
        uint16 i;
        for (i = 0; i < request_lenght; i++) 
            Data_Read_Buf[i] = request[i];
        Data_Read_Buf_Lenght = request_lenght;
        Data_Read_Buf_Flag  = 1;

    }
    /*DebugIfWriteString("U Data_Read_Buf: ");
    ArrayPrint(Data_Read_Buf, Data_Read_Buf_Lenght);
    DebugIfWriteString("\r\n");*/
}

extern uint8 BluetoothDataParce(uint8* request, uint16 request_lenght, uint8* responce, uint16 *responce_lenght)
{
    /*DebugWriteString("BluetoothDataParce: ");
    ArrayPrint(request, request_lenght);
    DebugWriteString("\r\n");*/

    uint8 secure_code = 4;

    AmountDataFlag = 1;
    AmountData[0] = '1';
    AmountDataLenght = 1;

    if ( AmountDataFlag == 0 ) 
        return 1;

    //Host
    if ( FindParameter(request, request_lenght, secure_code, Host, HOST_LEN, HOST_SECURE, HostData, &HostDataLenght) == 0) 
    {
        /*DebugWriteString("param: ");
        ArrayPrint(Host, HOST_LEN);
        DebugWriteString("\r\n");
        DebugWriteString("data: ");
        ArrayPrint(HostData, HostDataLenght);
        DebugWriteString("\r\n");*/
    }
    else
        return 3;

    //Path
    if ( FindParameter(request, request_lenght, secure_code, Path, PATH_LEN, PATH_SECURE, PathData, &PathDataLenght) == 0) 
    {
        /*DebugWriteString("param: ");
        ArrayPrint(Path, PATH_LEN);
        DebugWriteString("\r\n");
        DebugWriteString("data: ");
        ArrayPrint(PathData, PathDataLenght);
        DebugWriteString("\r\n");*/
    }
    else
        return 4;


    //Message
    if ( FindParameter(request, request_lenght, secure_code, Message, MESSAGE_LEN, MESSAGE_SECURE, MessageData, &MessageDataLenght) == 0) 
    {
        /*DebugWriteString("param: ");
        ArrayPrint(Path, PATH_LEN);
        DebugWriteString("\r\n");
        DebugWriteString("data: ");
        ArrayPrint(PathData, PathDataLenght);
        DebugWriteString("\r\n");*/
    }
    else
        return 5;



    ArrayClear(WiFi_Buf, WIFI_BUF_SIZE);
    WiFi_Buf_Pointer = 0;

    //Put to( WiFi Array: { 
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextBracketFLeft, TEXT_SYMBOL_LEN);



    //Put to WiFi Array: "url":"
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextUrl, TEXT_URL_LEN);
   
    //Put to WiFi Array: https:\\ + Host + Path
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextUrlHttps, TEXT_URL_HTTPS_LEN);
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, HostData, HostDataLenght);
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, PathData, PathDataLenght);

    //Put to WiFi Array: ",
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextApostropheDComa, TEXT_APOSTROPHE_D_COMA_LEN);


    //Put to WiFi Array: "post":{
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextPost, TEXT_POST_LEN);
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextNewStr, TEXT_NEW_STR_LEN);

    //Put to WiFi Array: "mobileAppMessage","
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextMessage, TEXT_MESSAGE_LEN);
    //message data
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, MessageData, MessageDataLenght);
    //Put to WiFi Array: ",
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextApostropheDComa, TEXT_APOSTROPHE_D_COMA_LEN);

    //Put to WiFi Array: }
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextBracketFRight, TEXT_SYMBOL_LEN);
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextNewStr, TEXT_NEW_STR_LEN);



    //Put to WiFi Array: }
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextBracketFRight, TEXT_SYMBOL_LEN);
    ArrayPut(WiFi_Buf, &WiFi_Buf_Pointer, TextNewStr, TEXT_NEW_STR_LEN);
    
    
    
    ArrayPrint(WiFi_Buf, WiFi_Buf_Pointer);
    //Send WiFi Array
    return 0;
}



extern uint8 CheckParameterData(uint8* data, uint16 data_len, uint8* check_data, uint16 check_data_len)
{
    if ( data_len != check_data_len ) return 1;

    uint16 i;
    uint8 chr;
    for (i=0; i<data_len; i++)
        if ( data[i] != check_data[i])
            return 2;

    return 0;
}

extern uint8 FindParameter(uint8* request, uint16 request_lenght, uint8 secure_code, 
    uint8* param, uint16 param_len, uint8 param_secure, uint8* param_data, uint16 *param_data_len)
{

    if ( secure_code < param_secure ) return 1;

    uint16 i;
    uint8 chr;

    uint8 array_flag = 0;
    uint8 data_level = 0;
    uint8 parameter_flag = 0;

    ArrayClear( Cmdd_Parameter, CMDD_PARAMETER_MAX_LENGHT );
    Cmdd_Parameter_Pointer = 0;
    ArrayClear( Cmdd_Data, CMDD_DATA_MAX_LENGHT );
    Cmdd_Data_Pointer = 0;

    for (i=0; i<request_lenght; i++)
    {
        chr = request[i];
        switch(chr)
        {
            case '\"':
            case '\r':
            case '\n':
                break;
            
            case '{':
                    if ( data_level == 0) 
                    {
                        parameter_flag = 1;
                    }
                    data_level++;
                break;

            case '}':
                    data_level--;
                    if ( data_level == 0 ) 
                    {
                        parameter_flag = 1;
                        

                        if ( CheckParameter( param, param_len, param_data, &*param_data_len, 
                            Cmdd_Parameter, Cmdd_Parameter_Pointer, Cmdd_Data, Cmdd_Data_Pointer ) == 0)
                        { 
                            return 0;
                        }          
                        
                        ArrayClear( Cmdd_Parameter, CMDD_PARAMETER_MAX_LENGHT );
                        Cmdd_Parameter_Pointer = 0;                                                
                        ArrayClear( Cmdd_Data, CMDD_DATA_MAX_LENGHT );
                        Cmdd_Data_Pointer = 0;
                    }
                break;

            case ',':
                    if ( data_level == 1 ) 
                    {
                        parameter_flag = 1;
                        if ( CheckParameter( param, param_len, param_data, &*param_data_len, 
                            Cmdd_Parameter, Cmdd_Parameter_Pointer, Cmdd_Data, Cmdd_Data_Pointer ) == 0)
                        {    
                            return 0;
                        }
                        
                        ArrayClear( Cmdd_Parameter, CMDD_PARAMETER_MAX_LENGHT );
                        Cmdd_Parameter_Pointer = 0;
                        ArrayClear( Cmdd_Data, CMDD_DATA_MAX_LENGHT );
                        Cmdd_Data_Pointer = 0;
                    }
                break;

            case '[':
                    array_flag++;
                break;

            case ']':
                    if (array_flag) array_flag--;
                break;

            case ':':
                    if (array_flag == 0)
                    {                   
                        if ( request[i+1] == ' ' ) i++;              
                        if ( data_level == 1 ) 
                        {
                            parameter_flag = 0;
                        }
                    }
                break;

            default:
                    if( parameter_flag ) {
                        if ( chr==' ' && data_level==1 && Cmdd_Parameter_Pointer==0){                        
                            //DebugWriteString("Space\r\n");
                        }
                        else
                            Cmdd_Parameter[Cmdd_Parameter_Pointer++] = chr;
                    }
                    else 
                        Cmdd_Data[Cmdd_Data_Pointer++] = chr;
                break;
        }
       
    }

    return 1;
}




extern uint8 CheckParameter( uint8* param, uint16 param_len, uint8* param_data, uint16 *param_data_len, 
    uint8* chk_param, uint16 chk_param_len, uint8* chk_data, uint16 chk_data_len)
{
    /*DebugWriteString("chk_param: ");
    ArrayPrint(chk_param, chk_param_len);
    DebugWriteString("\r\n");
    DebugWriteString("chk_data: ");
    ArrayPrint(chk_data, chk_data_len);
    DebugWriteString("\r\n");
    DebugWriteString("param: ");
    ArrayPrint(param, param_len);
    DebugWriteString("\r\n");
    DebugWriteString("\r\n");*/

    if ( param_len != chk_param_len ) return 1;

    uint16 i;
    uint8 chr;
    for (i=0; i<param_len; i++)
        if ( param[i] != chk_param[i])
            return 2;

    for (i=0; i<chk_data_len; i++)
        param_data[i] = chk_data[i];

    *param_data_len = chk_data_len;

    /*DebugWriteString("param: ");
    ArrayPrint(param, param_len);
    DebugWriteString("\r\n");
    DebugWriteString("data: ");
    ArrayPrint(chk_data, chk_data_len);
    DebugWriteString("\r\n");*/

    return 0;
}














/*----------------------------------------------------------------------------*
 *  NAME
 *      AppInit
 *
 *  DESCRIPTION
 *      This user application function is called after a power-on reset
 *      (including after a firmware panic), after a wakeup from Hibernate or
 *      Dormant sleep states, or after an HCI Reset has been requested.
 *
 *      NOTE: In the case of a power-on reset, this function is called
 *      after AppPowerOnReset().
 *
 *  PARAMETERS
 *      last_sleep_state [in]   Last sleep state
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void AppInit(sleep_state last_sleep_state)
{
    uint16 gatt_db_length = 0;  /* GATT database size */
    uint16 *p_gatt_db = NULL;   /* GATT database */
    
    //LedsInit();
    
    /* Initialise application debug */
    //DebugIfInit();
    ArrayClear(Request_Buf, REQUEST_BUF_SIZE);
    Request_Buf_Pointer = 0;
    Request_Read_Flag = 0;
    Request_Read_Attempts = 0;

    ArrayClear(Responce_Buf, RESPONCE_BUF_SIZE);
    Responce_Buf_Lenght = 0;

    ArrayClear(Data_Write_Buf, DATA_BUF_SIZE);         
    Data_Write_Buf_Lenght = 0;      
    Data_Write_Buf_Flag = 0;   
    Data_Write_Buf_Chunk_Lenght = 0;  
    Data_Write_Buf_Chunk_Pointer = 0;   

    ArrayClear(Data_Read_Buf, DATA_BUF_SIZE);
    Data_Read_Buf_Lenght = 0;  
    Data_Read_Buf_Flag = 0;
    Data_Read_Buf_Chunk_Lenght = 0;  
    Data_Read_Buf_Chunk_Pointer = 0;  

    AmountDataFlag = 0;

    

    /* Initialise communications */
    DebugInit(1, uartRxDataCallback, NULL);

    /* Wakeup on UART RX line */
    SleepWakeOnUartRX(TRUE);
    SleepModeChange(sleep_mode_never);

   /* Start the debug messages on the UART */
    //Danil
    DebugIfWriteString("{ble_status: ble_enable, id: 100012}\r\n");


    #if defined(USE_STATIC_RANDOM_ADDRESS) && !defined(PAIRING_SUPPORT)
        /* Use static random address for the application */
       // GapSetStaticAddress();
    #endif

    /* Initialise the GATT Server application state */
    BleStatusEnableFlag = 0;
    //g_app_data.state = app_state_init;

    /* Initialise the application timers */
    TimerInit(MAX_APP_TIMERS, (void*)app_timers);
    
    /* Initialise local timers */
    g_app_data.con_param_update_tid = TIMER_INVALID;
    g_app_data.app_tid = TIMER_INVALID;

    /* Initialise GATT entity */
    GattInit();

    /* Initialise GATT Server H/W */
    InitHardware();

    /* Install GATT Server support for the optional Write procedure.
     * This is mandatory only if the control point characteristic is supported. 
     */
    GattInstallServerWrite();

    

    #ifdef NVM_TYPE_EEPROM
        /* Configure the NVM manager to use I2C EEPROM for NVM store */
        NvmConfigureI2cEeprom();
    #elif NVM_TYPE_FLASH
        /* Configure the NVM Manager to use SPI flash for NVM store. */
        NvmConfigureSpiFlash();
    #endif /* NVM_TYPE_EEPROM */

    Nvm_Disable();

    /* Battery initialisation on chip reset */
    //BatteryInitChipReset();

    /* Initialize the GAP data. Needs to be done before readPersistentStore */
    GapDataInit();

    /* Read persistent storage */
    readPersistentStore();

    /* Tell Security Manager module what value it needs to initialise its
     * diversifier to.
     */
    SMInit(g_app_data.diversifier);
    
    /* Initialise hardware data */
    HwDataInit();

    /* Initialise application data structure */
    appDataInit();

    //Danil
    LsSetTransmitPowerLevel(0);
    /* Tell GATT about our database. We will get a GATT_ADD_DB_CFM event when
     * this has completed.
     */
    p_gatt_db = GattGetDatabase(&gatt_db_length);

    GattAddDatabaseReq(gatt_db_length, p_gatt_db);
    
    //DebugWriteString("\r\n\r\nCSR:  AppInit end\r\n");

} 


#ifdef ENABLE_FOLD 
	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      AppProcesSystemEvent
	 *
	 *  DESCRIPTION
	 *      This user application function is called whenever a system event, such
	 *      as a battery low notification, is received by the system.
	 *
	 *  PARAMETERS
	 *      id   [in]               System event ID
	 *      data [in]               Event data
	 *
	 *  RETURNS
	 *      Nothing
	 *----------------------------------------------------------------------------*/
	void AppProcessSystemEvent(sys_event_id id, void *data)
	{
	    switch(id)
	    {
	        case sys_event_battery_low:
	        {
	            /* Battery low event received - notify the connected host. If 
	             * not connected, the battery level will get notified when 
	             * device gets connected again
	             */
	            if(g_app_data.state == app_state_connected)
	            {
	                //BatteryUpdateLevel(g_app_data.st_ucid);
	            }
	        }
	        break;

	        case sys_event_pio_changed:
	        {
	             /* Handle the PIO changed event. */
	             HandlePIOChangedEvent((pio_changed_data*)data);
	        }
	        break;
	            
	        default:
	            /* Ignore anything else */
	        break;
	    }
	}

	/*----------------------------------------------------------------------------*
	 *  NAME
	 *      AppProcessLmEvent
	 *
	 *  DESCRIPTION
	 *      This user application function is called whenever a LM-specific event
	 *      is received by the system.
	 *
	 *  PARAMETERS
	 *      event_code [in]         LM event ID
	 *      event_data [in]         LM event data
	 *
	 *  RETURNS
	 *      TRUE if the app has finished with the event data; the control layer
	 *      will free the buffer.
	 *----------------------------------------------------------------------------*/
	bool AppProcessLmEvent(lm_event_code event_code, LM_EVENT_T *p_event_data)
	{
	    switch (event_code)
	    {
	        /* Handle events received from Firmware */        

	        case GATT_ADD_DB_CFM:
	            //LedsGreenOn();
	            //DebugIfWriteString("\r\nCSR: GATT_ADD_DB_CFM");
	            /* Attribute database registration confirmation */
	            handleSignalGattAddDbCfm((GATT_ADD_DB_CFM_T*)p_event_data);
	        break;

	        case LM_EV_CONNECTION_COMPLETE:
	            //DebugIfWriteString("\r\nCSR: LM_EV_CONNECTION_COMPLETE");
	            /* Handle the LM connection complete event. */
	            handleSignalLmEvConnectionComplete(
	                                (LM_EV_CONNECTION_COMPLETE_T*)p_event_data);
	        break;

	        case GATT_CANCEL_CONNECT_CFM:
	            //DebugIfWriteString("\r\nCSR: GATT_CANCEL_CONNECT_CFM");
	            /* Confirmation for the completion of GattCancelConnectReq()
	             * procedure 
	             */
	            handleSignalGattCancelConnectCfm();
	        break;

	        case GATT_CONNECT_CFM:
	            //LedsRedOn();
	            //DebugIfWriteString("{ble_status: device_connected}\r\n");
	            //DebugIfWriteString("\r\nCSR: GATT_CONNECT_CFM");
	            /* Confirmation for the completion of GattConnectReq() 
	             * procedure
	             */
	            handleSignalGattConnectCfm((GATT_CONNECT_CFM_T*)p_event_data);
	        break;

	        case SM_KEYS_IND:
	            //DebugIfWriteString("\r\nCSR: SM_KEYS_IND");
	            /* Indication for the keys and associated security information
	             * on a connection that has completed Short Term Key Generation 
	             * or Transport Specific Key Distribution
	             */
	            handleSignalSmKeysInd((SM_KEYS_IND_T *)p_event_data);
	        break;

	        case SM_SIMPLE_PAIRING_COMPLETE_IND:
	            //DebugIfWriteString("\r\nCSR: SM_SIMPLE_PAIRING_COMPLETE_IND");
	            /* Indication for completion of Pairing procedure */
	            handleSignalSmSimplePairingCompleteInd(
	                (SM_SIMPLE_PAIRING_COMPLETE_IND_T *)p_event_data);
	        break;

	        case LM_EV_ENCRYPTION_CHANGE:
	            //DebugIfWriteString("\r\nCSR: LM_EV_ENCRYPTION_CHANGE");
	            /* Indication for encryption change event */
	        break;


	        case SM_DIV_APPROVE_IND:
	            //DebugIfWriteString("\r\nCSR: SM_DIV_APPROVE_IND");
	            /* Indication for SM Diversifier approval requested by F/W when 
	             * the last bonded host exchange keys. Application may or may not
	             * approve the diversifier depending upon whether the application 
	             * is still bonded to the same host
	             */
	            handleSignalSmDivApproveInd((SM_DIV_APPROVE_IND_T *)p_event_data);
	        break;


	        /* Received in response to the LsConnectionParamUpdateReq() 
	         * request sent from the slave after encryption is enabled. If 
	         * the request has failed, the device should send the same 
	         * request again only after Tgap(conn_param_timeout). Refer Bluetooth 4.0 
	         * spec Vol 3 Part C, Section 9.3.9 and HID over GATT profile spec 
	         * section 5.1.2.
	         */
	        case LS_CONNECTION_PARAM_UPDATE_CFM:
	            //LedsRedOn();
	            //DebugIfWriteString("\r\nCSR: LS_CONNECTION_PARAM_UPDATE_CFM");
	            handleSignalLsConnParamUpdateCfm(
	                            (LS_CONNECTION_PARAM_UPDATE_CFM_T*)p_event_data);
	        break;

	        case LS_CONNECTION_PARAM_UPDATE_IND:
	            //LedsRedOn();
	            //DebugIfWriteString("\r\nCSR: LS_CONNECTION_PARAM_UPDATE_IND");
	            /* Indicates completion of remotely triggered Connection 
	             * Parameter Update procedure
	             */
	            handleSignalLsConnParamUpdateInd(
	                            (LS_CONNECTION_PARAM_UPDATE_IND_T *)p_event_data);

	        break;

	        case GATT_ACCESS_IND:
	            //LedsBlueOn();
	            //DebugIfWriteString("\r\nCSR: GATT_ACCESS_IND");
	            /* Indicates that an attribute controlled directly by the
	             * application (ATT_ATTR_IRQ attribute flag is set) is being 
	             * read from or written to.
	             */
	            handleSignalGattAccessInd((GATT_ACCESS_IND_T *)p_event_data);
	        break;

	        case GATT_DISCONNECT_IND:
	            //LedsGreenOn();
	            //DebugIfWriteString("\r\nCSR: GATT_DISCONNECT_IND");
	            DebugIfWriteString("{ble_status: device_disconnected}\r\n");
	            /* Disconnect procedure triggered by remote host or due to 
	             * link loss is considered complete on reception of 
	             * LM_EV_DISCONNECT_COMPLETE event. So, it gets handled on 
	             * reception of LM_EV_DISCONNECT_COMPLETE event.
	             */
	        break;

	        case GATT_DISCONNECT_CFM:
	            //LedsGreenOn();
	        	//DebugIfWriteString("\r\nCSR: GATT_DISCONNECT_CFM");
	            DebugIfWriteString("{ble_status: device_disconnected}\r\n");
	            //DebugIfWriteString("\r\nCSR: GATT_DISCONNECT_CFM");
	            /* Confirmation for the completion of GattDisconnectReq()
	             * procedure is ignored as the procedure is considered complete 
	             * on reception of LM_EV_DISCONNECT_COMPLETE event. So, it gets 
	             * handled on reception of LM_EV_DISCONNECT_COMPLETE event.
	             */
	        break;

	        case LM_EV_DISCONNECT_COMPLETE:
	        {
	            //LedsGreenOn();
	            //DebugIfWriteString("\r\nCSR: LM_EV_DISCONNECT_COMPLETE");

	            /* Disconnect procedures either triggered by application or remote
	             * host or link loss case are considered completed on reception 
	             * of LM_EV_DISCONNECT_COMPLETE event
	             */
	             handleSignalLmDisconnectComplete(
	                    &((LM_EV_DISCONNECT_COMPLETE_T *)p_event_data)->data);
	        }
	        break;

	        default:
	            //DebugIfWriteString("\r\nCSR: DEFAULT   event_code: ");
	            //DebugIfWriteUint16(event_code);
	            /* Ignore any other event */ 
	        break;

	    }

	    return TRUE;
	}

#endif



