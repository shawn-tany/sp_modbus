#ifndef MB_RTU
#define MB_RTU

#include "mb_common.h"

#define MBRTU_RECV_DELAY    200
#define MBRTU_RECV_TIMEOUT  1000
#define MBRTU_SERIAL_SIZE   32

typedef struct 
{
    UINT16_T baudrate;
    UINT16_T databit;
    UINT16_T stopbit;
    UINT16_T flowctl;
    UINT16_T parity;
    char     serial[MBRTU_SERIAL_SIZE];
} MBRTU_CTL_T;

/*
 * Function  : Create a ModBus RTU descriptor
 * rtu_ctl   : configure parameters of ModBus RTU
 * return    : (MB_DESC_T *)=SUCCESS NULL=ERRROR
 */
MB_DESC_T *mb_rtu_init(MBRTU_CTL_T *rtu_ctl);

/*
 * Function  : close a ModBus RTU descriptor
 * mb_desc   : the ModBus RTU descriptor you want to close
 * return    : void
 */
void mb_rtu_close(MB_DESC_T *mb_desc);

/*
 * Function  : send ModBus RTU data from ModBus cache to slaver
 * mb_desc   : ModBus RTU descriptor
 * mb_data   : ModBus cache
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_rtu_send(MB_DESC_T *mb_desc, MB_DATA_T *mb_data);

/*
 * Function  : recv ModBus RTU data from slaver to ModBus cache
 * mb_desc   : ModBus RTU descriptor
 * mb_data   : ModBus cache
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_rtu_recv(MB_DESC_T *mb_desc, MB_DATA_T *mb_data);

#endif
