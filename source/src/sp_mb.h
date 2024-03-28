/*
 * Author   : shawn-tany
 * Function : 1. Open/Close connection with ModBus slaver
 *            2. Recv/Send  data to ModBus slaver
 */

#ifndef SP_MODBUS
#define SP_MODBUS

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
} SPMB_CTL_T;

typedef struct 
{
    pthread_mutex_t lock;
    pthread_t       pid;
    UINT8_T         stay;
} SPMB_RESC_T;

typedef struct
{
    UINT16_T    i_addr_start;
    UINT16_T    i_number;
    UINT16_T    o_addr_start;
    UINT16_T    o_number;
} SPMB_IOCONF_T;

typedef union
{
    MBTCP_DATA_T tcp_data;
    MBRTU_DATA_T rtu_data;
} SPMB_DATA_T;

typedef struct 
{
    MB_TYPE_T        mb_type;
    SPMB_RESC_T    mb_resc;
    SPMB_IOCONF_T  mb_ioconf;

    union
    {
        MBTCP_CTX_T *mb_tcp_ctx;
        MBRTU_CTX_T *mb_rtu_ctx;
    } ctx;
} SPMB_CTX_T;

/*
 * Function  : Create a ModBus Context
 * mb_ctl    : some information for create modBus Context
 * return    : (SPMB_CTX_T *)=SUCCESS NULL=ERRROR
 */
SPMB_CTX_T *sp_mb_init(SPMB_CTL_T *mb_ctl);

/*
 * Function : close a ModBus Context
 * mb_ctx   : the ModBus context you want to close
 * return   : void
 */
void sp_mb_close(SPMB_CTX_T *mb_ctx);

/*
 * Function : recv ModBus data from slaver
 * mb_ctx   : ModBus context
 * return   : 0=CLOSE length=SUCCESS -1=ERROR
 */
int sp_mb_recv(SPMB_CTX_T *mb_ctx, MB_INFO_T *mb_info);

/*
 * Function : send ModBus data to slaver
 * mb_ctx   : ModBus context
 * return   : 0=CLOSE length=SUCCESS -1=ERROR
 */
int sp_mb_send(SPMB_CTX_T *mb_ctx, MB_INFO_T *mb_info);

int sp_mbio_get(SPMB_CTX_T *mb_ctx, IO_DIRECTION_T direction, 
    UINT16_T ioidx, IO_STATUS_T *status);

int sp_mbio_set(SPMB_CTX_T *mb_ctx, UINT16_T ioidx, IO_STATUS_T statu);

/*
 * Function  : show response status from ModBus slaver
 * mb_info   : ModBus master info
 * return    : void
 */
void sp_mb_status_show(MB_INFO_T mb_info);

/*
 * Function  : show ModBus data after decap
 * mb_info   : ModBus master info
 * return    : void
 */
void sp_mb_data_show(MB_INFO_T mb_info);

#endif
