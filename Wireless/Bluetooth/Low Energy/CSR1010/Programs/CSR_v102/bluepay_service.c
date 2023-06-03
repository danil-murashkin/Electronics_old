/*****************************************************************************
 *  Copyright (c) 2017 bluePay
 *  Part of CSR uEnergy SDK 2.6.0
 *  Application version 2.6.0.0
 *
 *  FILE
 *      bluepay_service.c
 *
 *  DESCRIPTION
 *      This file defines routines for using the bluepay service.
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header files
 *============================================================================*/

#include <gatt.h>           /* GATT application interface */
#include <bluetooth.h>      /* Bluetooth specific type definitions */
#include <config_store.h>   /* Interface to the Configuration Store */
#include <mem.h>
#include <string.h>
#include <buf_utils.h>

/*============================================================================*
 *  Local Header files
 *============================================================================*/

#include "app_gatt_db.h"         /* GATT database definitions */
#include "bluepay_service.h"     /* Interface to this file */
#include "debug_interface.h"
#include "gatt_server.h"

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Device Information Service System ID characteristic */
/* Bytes have been reversed */
#define SYSTEM_ID_FIXED_CONSTANT    (0xFFFE)
#define SYSTEM_ID_LENGTH            (8)

/*============================================================================*
 *  Private Datatypes
 *============================================================================*/

/* System ID : System ID has two fields;
 * Manufacturer Identifier           : The Company Identifier is concatenated 
 *                                     with 0xFFFE
 * Organizationally Unique Identifier: Company Assigned Identifier of the
 *                                     Bluetooth Address
 */


/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/
static uint8 confirm_text[7] = {'c','o','n','f','i','r','m'};

/*----------------------------------------------------------------------------*
 *  NAME
 *      BluePayHandleAccessWrite
 *
 *  DESCRIPTION
 *      This function handles read operations on bluePay service
 *      attributes maintained by the application and responds with the
 *      GATT_ACCESS_RSP message.
 *
 *  PARAMETERS
 *      p_ind [in]              Data received in GATT_ACCESS_IND message.
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void BluePayHandleAccessWrite(GATT_ACCESS_IND_T *p_ind)
{
    uint16 length = 0;                  /* Length of attribute data, octets */
    uint8 *p_value = NULL;              /* Pointer to attribute value */
    sys_status rc = sys_status_success;

    uint8 int_array[DATA_UINT_LEN]; 
    uint8 int_array_len;
    uint16 i;

    //DebugIfWriteString("\r\nCSR:         BluePayHandleAccessWrite");

    switch(p_ind->handle)
    {
        case HANDLE_BLUEPAY_DATA_WRITE_LENGHT:
        {
            //DebugIfWriteString("\r\nCSR:             BLUEPAY_DATA_WRITE_LENGHT\r\n");
            Data_Read_Buf_Flag = 0;
            Data_Write_Buf_Flag = 1;
            Data_Read_Buf_Lenght = 0;
            if(!Data_Read_Buf_Flag) 
            {
                Data_Write_Buf_Chunk_Pointer = 0;
                Data_Write_Buf_Lenght = 0;
                Data_Write_Buf_Chunk_Lenght = StrToInt(p_ind->value, p_ind->size_value);
                if (Data_Write_Buf_Chunk_Lenght) {
                    ArrayClear(Data_Write_Buf, DATA_BUF_SIZE);                
                    Data_Write_Buf_Flag = 1;                
                    ArrayClear(Data_Read_Buf, DATA_BUF_SIZE);
                    Data_Read_Buf_Flag  = 0;
                }
        
                /* DebugIfWriteString("Data_Write_Buf_Chunk_Lenght: ");
                DebugIfWriteInt(Data_Write_Buf_Chunk_Lenght);
                DebugIfWriteString("\r\n");*/
            }
        }
        break;

        case HANDLE_BLUEPAY_DATA_WRITE:
        {
            //DebugIfWriteString("\r\nCSR:             BLUEPAY_DATA_WRITE\r\n");
            
            if(Data_Write_Buf_Flag) 
            {     
                ArrayPut(Data_Write_Buf, &Data_Write_Buf_Lenght, p_ind->value, p_ind->size_value);

                Data_Write_Buf_Chunk_Pointer++;
                if (Data_Write_Buf_Chunk_Pointer >= Data_Write_Buf_Chunk_Lenght) {
                    Data_Write_Buf_Flag = 0;

                    //BluetoothDataParce(Data_Write_Buf, Data_Write_Buf_Lenght, Data_Read_Buf, &Data_Read_Buf_Lenght);

                    DebugIfWriteString("{ble_status: data_write_finish, ble_data: {");
                    ArrayPrint(Data_Write_Buf, Data_Write_Buf_Lenght);
                    DebugIfWriteString("} }\r\n");

                    /*DebugIfWriteString("Data_Write_Buf: ");
                    ArrayPrint(Data_Write_Buf, Data_Write_Buf_Lenght);
                    DebugIfWriteString("\r\n");*/
                }
            }
        }
        break;


        case HANDLE_BLUEPAY_DATA_READ_POINTER:
        {
            //DebugIfWriteString("\r\nCSR:             BLUEPAY_DATA_READ_POINTER\r\n");

            if(Data_Read_Buf_Flag)
            {
                Data_Read_Buf_Chunk_Pointer = StrToInt(p_ind->value, p_ind->size_value);
                /*DebugIfWriteString("Data_Read_Buf_Chunk_Pointer: ");
                DebugIfWriteInt(Data_Read_Buf_Chunk_Pointer);
                DebugIfWriteString("\r\n");*/
            }

        }
        break;

        
        default:
        {
            rc = gatt_status_write_not_permitted;
        }
        break;
    }

    /* Send response indication */
    GattAccessRsp(p_ind->cid, p_ind->handle, rc, length, p_value);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BluePayHandleAccessRead
 *
 *  DESCRIPTION
 *      This function handles read operations on Device Information Service
 *      attributes maintained by the application and responds with the
 *      GATT_ACCESS_RSP message.
 *
 *  PARAMETERS
 *      p_ind [in]              Data received in GATT_ACCESS_IND message.
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void BluePayHandleAccessRead(GATT_ACCESS_IND_T *p_ind)
{

    uint16 length = 0;                  /* Length of attribute data, octets */
    uint8 *p_value = NULL;              /* Pointer to attribute value */
    sys_status rc = sys_status_success;

    uint8 int_array[DATA_UINT_LEN]; 
    uint8 int_array_len;

    uint16 answ_len;

    uint16 i;

    //DebugIfWriteString("\r\nCSR:         BluePayHandleAccessRead");
        

    switch(p_ind->handle)
    {
        case HANDLE_BLUEPAY_DATA_READ:
        {
            //DebugIfWriteString("\r\nCSR:             BLUEPAY_DATA_READ\r\n");

            answ_len = 0;
            ArrayClear(Data_Chunk_Buf, DATA_CHUNK_LENGHT);

            /*DebugIfWriteString("Data_Write_Buf_Flag: ");
            DebugIfWriteInt(Data_Write_Buf_Flag);
            DebugIfWriteString("\r\n");
            DebugIfWriteString("Data_Read_Buf_Flag: ");
            DebugIfWriteInt(Data_Read_Buf_Flag);
            DebugIfWriteString("\r\n");
            DebugIfWriteString("Data_Read_Buf_Chunk_Pointer: ");
            DebugIfWriteInt(Data_Read_Buf_Chunk_Pointer);
            DebugIfWriteString("\r\n");
            DebugIfWriteString("Data_Read_Buf_Chunk_Lenght: ");
            DebugIfWriteInt(Data_Read_Buf_Chunk_Lenght);
            DebugIfWriteString("\r\n");
            DebugIfWriteString("Data_Read_Buf_Lenght: ");
            DebugIfWriteInt(Data_Read_Buf_Lenght);
            DebugIfWriteString("\r\n");
            DebugIfWriteString("Data_Read_Buf_Chunk_Pointer*DATA_CHUNK_LENGHT: ");
            DebugIfWriteInt( Data_Read_Buf_Chunk_Pointer*DATA_CHUNK_LENGHT);
            DebugIfWriteString("\r\n");*/

            if ( (!Data_Write_Buf_Flag) && (Data_Read_Buf_Flag) && (Data_Read_Buf_Chunk_Pointer) 
                && (Data_Read_Buf_Chunk_Pointer <= Data_Read_Buf_Chunk_Lenght))
            {    
                ArrayPutArray(Data_Chunk_Buf, DATA_CHUNK_LENGHT, Data_Read_Buf, Data_Read_Buf_Lenght, 
                    0, ((Data_Read_Buf_Chunk_Pointer-1)*DATA_CHUNK_LENGHT), DATA_CHUNK_LENGHT);

                if (Data_Read_Buf_Chunk_Pointer == Data_Read_Buf_Chunk_Lenght) 
                {
                    answ_len = Data_Read_Buf_Lenght - (Data_Read_Buf_Chunk_Pointer-1)*DATA_CHUNK_LENGHT;
                    DebugIfWriteString("{ble_status: data_read_finish}\r\n");
                }
                else 
                    answ_len = DATA_CHUNK_LENGHT;

                Data_Read_Buf_Chunk_Pointer ++;

                /*DebugIfWriteString("Data_Read_Buf_Chunk_Pointer:");
                DebugIfWriteInt(Data_Read_Buf_Chunk_Pointer);
                DebugIfWriteString("\r\n");*/
                /*DebugIfWriteString("Data_Chunk_Buf:");
                ArrayPrint(Data_Chunk_Buf, DATA_CHUNK_LENGHT);
                DebugIfWriteString("\r\n");*/
            }
            else
            {
                answ_len = 0;
            }

            length  = answ_len - p_ind->offset;
            p_value = Data_Chunk_Buf + p_ind->offset;

        }
        break;

        case HANDLE_BLUEPAY_DATA_READ_LENGHT:
        {
            //DebugIfWriteString("\r\nCSR:             BLUEPAY_DATA_READ_LENGHT\r\n");
            
            int_array_len=0;
            ArrayClear(int_array, DATA_UINT_LEN);
            
            /*DebugIfWriteString("B Data_Read_Buf_Lenght: ");
            DebugIfWriteInt(Data_Read_Buf_Lenght);
            DebugIfWriteString("\r\n");            
            
            DebugIfWriteString("B Data_Read_Buf: ");
            ArrayPrint(Data_Read_Buf, Data_Read_Buf_Lenght);
            DebugIfWriteString("\r\n");*/

            if (Data_Write_Buf_Flag) 
            {
                IntToStr(0, int_array, &int_array_len);
            } 
            else 
            {
                if ( Data_Read_Buf_Lenght> 3 )
                {
                    //DebugIfWriteString("RDL Data_Read_Buf: asdasdasd\r\n");
                    Data_Read_Buf_Chunk_Lenght = Data_Read_Buf_Lenght / DATA_CHUNK_LENGHT;
                    if (Data_Read_Buf_Lenght % DATA_CHUNK_LENGHT) 
                        Data_Read_Buf_Chunk_Lenght += 1;
                    
                    Data_Read_Buf_Chunk_Pointer = 1;
                 
                    /*DebugIfWriteString("Data_Read_Buf_Chunk_Lenght: ");
                    DebugIfWriteInt(Data_Read_Buf_Chunk_Lenght);
                    DebugIfWriteString("\r\n");*/

                    IntToStr(Data_Read_Buf_Chunk_Lenght, int_array, &int_array_len);

                    Data_Read_Buf_Chunk_Pointer = 1;
                    Data_Write_Buf_Flag = 0;
                    Data_Read_Buf_Flag = 1;
                }
                else
                {
                    IntToStr(0, int_array, &int_array_len);
                    Data_Read_Buf_Flag = 0;
                }       
            }

            length  = int_array_len - p_ind->offset;
            p_value = int_array + p_ind->offset;

        }
        break;

        default:
        {
            rc = gatt_status_read_not_permitted;
        }
        break;
    }

    /* Send response indication */
    GattAccessRsp(p_ind->cid, p_ind->handle, rc, length, p_value);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BluePayCheckHandleRange
 *
 *  DESCRIPTION
 *      This function is used to check if the handle belongs to the Device 
 *      Information Service.
 *
 *  PARAMETERS
 *      handle [in]             Handle to check
 *
 *  RETURNS
 *      TRUE if handle belongs to the Device Information Service, FALSE
 *      otherwise
 *----------------------------------------------------------------------------*/
extern bool BluePayCheckHandleRange(uint16 handle)
{
    return ((handle >= HANDLE_BLUEPAY_SERVICE) &&
            (handle <= HANDLE_BLUEPAY_SERVICE_END))
            ? TRUE : FALSE;
}
