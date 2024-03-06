#ifndef MB_RTU
#define MB_RTU

#include "mb_common.h"

#define MBRTU_RECV_DELAY    200
#define MBRTU_RECV_TIMEOUT  100
#define MBRTU_SERIAL_SIZE   32

typedef struct 
{
    UINT16_T max_data_size;

    UINT16_T slaver_addr;
    UINT16_T baudrate;
    UINT16_T databit;
    UINT16_T stopbit;
    UINT16_T flowctl;
    UINT16_T parity;
    char     serial[MBRTU_SERIAL_SIZE];
} MBRTU_CTL_T;

typedef struct
{
    int com_fd;
} __attribute__((packed)) MBRTU_DESC_T;

typedef struct 
{
    UINT16_T slaver_addr;
    UINT16_T crc_checksum;
} __attribute__((packed)) MBRTU_INFO_T;

typedef struct
{
    MBRTU_INFO_T rtu_info[MB_DIRECT_NUM];
    MB_DATA_T   *mb_data;
} __attribute__((packed)) MBRTU_DATA_T;

typedef struct 
{
    MBRTU_DESC_T mb_rtu_desc;
    MBRTU_DATA_T mb_rtu_data;
} __attribute__((packed)) MBRTU_CTX_T;

/*
 * Function  : Create a ModBus RTU context
 * rtu_ctl   : configure parameters of ModBus RTU
 * return    : (MBRTU_CTX_T *)=SUCCESS NULL=ERRROR
 */
MBRTU_CTX_T *mb_rtu_init(MBRTU_CTL_T *rtu_ctl);

/*
 * Function  : close a ModBus RTU context
 * mbrtu_ctx : the ModBus RTU context you want to close
 * return    : void
 */
void mb_rtu_close(MBRTU_CTX_T *mbrtu_ctx);

/*
 * Function  : send ModBus RTU data from ModBus cache to slaver
 * mbrtu_ctx : ModBus RTU context
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_rtu_send(MBRTU_CTX_T *mbrtu_ctx);

/*
 * Function  : recv ModBus RTU data from slaver to ModBus cache
 * mbrtu_ctx : ModBus RTU context
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_rtu_recv(MBRTU_CTX_T *mbrtu_ctx);

void mbrtuctx_info_updata(MBRTU_CTX_T *mbrtu_ctx, MB_INFO_T *mb_info);

void mbrtuctx_info_takeout(MBRTU_CTX_T *mbrtu_ctx, MB_INFO_T *mb_info);

#endif
