/*
 * Author   : tanxiaoyang
 * Company  : EVOC
 * Function : 1. Open/Close connection with ModBus slaver
 *            2. Recv/Send  data to ModBus slaver
 */

#ifndef EVOC_MODBUS
#define EVOC_MODBUS

#include <pthread.h>

#include "mb_tcp.h"
#include "mb_rtu.h"

typedef enum
{
    IO_OFF = 0,
    IO_ON  = 1,
} IO_STATUS_T;

typedef enum
{
    IO_INPUT  = 0,
    IO_OUTPUT = 1,
} IO_DIRECTION_T;

typedef struct 
{    
    MB_TYPE_T   mb_type;
    char        mb_conf[128];
    MBTCP_CTL_T tcp_ctrl;
    MBRTU_CTL_T rtu_ctrl;
} EVOCMB_CTL_T;

typedef struct 
{
    pthread_mutex_t lock;
    pthread_t       pid;
    UINT8_T         stay;
} EVOCMB_RESC_T;

typedef struct
{
    UINT16_T    i_addr_start;
    UINT16_T    i_number;
    UINT16_T    o_addr_start;
    UINT16_T    o_number;
} EVOCMB_IOCONF_T;

typedef union
{
    MBTCP_DATA_T tcp_data;
    MBRTU_DATA_T rtu_data;
} EVOCMB_DATA_T;

typedef struct 
{
    MB_TYPE_T        mb_type;
    EVOCMB_RESC_T    mb_resc;
    EVOCMB_IOCONF_T  mb_ioconf;

    union
    {
        MBTCP_CTX_T *mb_tcp_ctx;
        MBRTU_CTX_T *mb_rtu_ctx;
    } ctx;
} EVOCMB_CTX_T;

/*
 * Function  : Create a ModBus Context
 * mb_ctl    : some information for create modBus Context
 * return    : (EVOCMB_CTX_T *)=SUCCESS NULL=ERRROR
 */
EVOCMB_CTX_T *evoc_mb_init(EVOCMB_CTL_T *mb_ctl);

/*
 * Function : close a ModBus Context
 * mb_ctx   : the ModBus context you want to close
 * return   : void
 */
void evoc_mb_close(EVOCMB_CTX_T *mb_ctx);

/*
 * Function : recv ModBus data from slaver
 * mb_ctx   : ModBus context
 * return   : 0=CLOSE length=SUCCESS -1=ERROR
 */
int evoc_mb_recv(EVOCMB_CTX_T *mb_ctx, MB_INFO_T *mb_info);

/*
 * Function : send ModBus data to slaver
 * mb_ctx   : ModBus context
 * return   : 0=CLOSE length=SUCCESS -1=ERROR
 */
int evoc_mb_send(EVOCMB_CTX_T *mb_ctx, MB_INFO_T *mb_info);

int evoc_mbio_get(EVOCMB_CTX_T *mb_ctx, IO_DIRECTION_T direction, 
    UINT16_T ioidx, IO_STATUS_T *status);

int evoc_mbio_set(EVOCMB_CTX_T *mb_ctx, UINT16_T ioidx, IO_STATUS_T statu);

/*
 * Function  : show response status from ModBus slaver
 * mb_info   : ModBus master info
 * return    : void
 */
void evoc_mb_status_show(MB_INFO_T mb_info);

/*
 * Function  : show ModBus data after decap
 * mb_info   : ModBus master info
 * return    : void
 */
void evoc_mb_data_show(MB_INFO_T mb_info);

#endif
