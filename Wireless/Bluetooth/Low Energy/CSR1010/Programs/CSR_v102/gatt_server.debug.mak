###########################################################
# Makefile generated by xIDE for uEnergy                   
#                                                          
# Project: gatt_server
# Configuration: Debug
# Generated: Sun 14. Oct 16:04:27 2018
#                                                          
# WARNING: Do not edit this file. Any changes will be lost 
#          when the project is rebuilt.                    
#                                                          
###########################################################

XIDE_PROJECT=gatt_server
XIDE_CONFIG=Debug
OUTPUT=gatt_server
OUTDIR=C:/Users/danil/GoogleDrive/Workspace/140_IoT_bluegate/Programs/CSR_v002
DEFS=

OUTPUT_TYPE=0
USE_FLASH=0
ERASE_NVM=1
CSFILE_CSR101x_A05=gatt_server_csr101x_A05.keyr
MASTER_DB=app_gatt_db.db
LIBPATHS=
INCPATHS=
STRIP_SYMBOLS=0
OTAU_BOOTLOADER=0
OTAU_CSFILE=
OTAU_NAME=
OTAU_SECRET=
OTAU_VERSION=7

DBS=\
\
      app_gatt_db.db\
      dev_info_service_db.db\
      gap_service_db.db\
      gatt_service_db.db\
      bluepay_service_db.db

INPUTS=\
      gap_service.c\
      nvm_access.c\
      dev_info_service.c\
      gatt_access.c\
      hw_access.c\
      gatt_server.c\
      debug_interface.c\
      bluepay_service.c\
      leds.c\
      $(DBS)

KEYR=\
      gatt_server_csr101x_A05.keyr

# Project-specific options
hw_version=0

-include gatt_server.mak
include $(SDK)/genmakefile.uenergy
