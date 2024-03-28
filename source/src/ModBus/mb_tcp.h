/*
 * Author   : shawn-tany
 * Function : 1. Open/Close connection with ModBus TCP server 
 *            2. Recv/Send  data to ModBus TCP server 
 */

#ifndef MB_TCP
#define MB_TCP

#include "mb_common.h"

#define MBTCP_ETHDEV_LEN    32
#define MBTCP_IPADDR_LEN    32
#define MBTCP_RECV_DELAY    200
#define MBTCP_CONN_DELAY    200
#define MBTCP_RECV_TIMEOUT  100
#define MBTCP_CONN_TIMEOUT  10

typedef struct 
{
    UINT8_T  unitid;
    UINT16_T max_data_size;
    UINT16_T port;
    char     ip[MBTCP_IPADDR_LEN];
    char     ethdev[MBTCP_ETHDEV_LEN];
} MBTCP_CTL_T;

typedef struct 
{
    int      socket;
    UINT16_T port;
    char     ip[MBTCP_IPADDR_LEN];
    char     ethdev[MBTCP_ETHDEV_LEN];
} MBTCP_DESC_T;

typedef struct 
{
    UINT16_T transaction_code;
    UINT16_T protocol_code;
    UINT16_T data_length;
    UINT8_T  unit_code;
} __attribute__((packed)) MBAP_HEAD_T;

typedef struct 
{
    MBAP_HEAD_T mbap_head[MB_DIRECT_NUM];
    MB_DATA_T  *mb_data;
} __attribute__((packed)) MBTCP_DATA_T;

typedef struct
{
    MBTCP_DESC_T mb_tcp_desc;
    MBTCP_DATA_T mb_tcp_data;
} MBTCP_CTX_T;

/*
 * Function  : Create a ModBus TCP context
 * rtu_ctl   : configure parameters of ModBus TCP
 * return    : (MBTCP_CTX_T *)=SUCCESS NULL=ERRROR
 */
MBTCP_CTX_T *mb_tcp_init(MBTCP_CTL_T *tcp_ctl);

/*
 * Function  : close a ModBus TCP context
 * mbtcp_ctx   : the ModBus TCP context you want to close
 * return    : void
 */
void mb_tcp_close(MBTCP_CTX_T *mbtcp_ctx);

/*
 * Function  : send ModBus TCP data from ModBus cache to slaver
 * mbtcp_ctx : ModBus TCP context
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_tcp_send(MBTCP_CTX_T *mbtcp_ctx);

/*
 * Function  : recv ModBus TCP data from slaver to ModBus cache
 * mbtcp_ctx   : ModBus TCP context
 * mb_data   : ModBus cache
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_tcp_recv(MBTCP_CTX_T *mbtcp_ctx);

void mbtcpctx_info_updata(MBTCP_CTX_T *mbtcp_ctx, MB_INFO_T *mb_info);

void mbtcpctx_info_takeout(MBTCP_CTX_T *mbtcp_ctx, MB_INFO_T *mb_info);

#endif
