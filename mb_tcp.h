/*
 * Author   : tanxiaoyang
 * Company  : EVOC
 * Function : 1. Open/Close connection with ModBus TCP server 
 *            2. Recv/Send  data to ModBus TCP server 
 */

#ifndef MB_TCP
#define MB_TCP

#include "mb_common.h"

#define MBTCP_ETHDEV_LEN    32
#define MBTCP_IPADDR_LEN    32
#define MBTCP_RECV_DELAY    200
#define MBTCP_RECV_TIMEOUT  100

typedef struct 
{
    UINT16_T port;
    char     ip[MBTCP_IPADDR_LEN];
    char     ethdev[MBTCP_ETHDEV_LEN];
} MBTCP_CTL_T;

typedef struct 
{
    MB_WORD_T transaction_code;
    MB_WORD_T protocol_code;
    MB_WORD_T data_length;
    UINT8_T   unit_code;
} __attribute__((packed)) MBAP_HEAD_T;

/*
 * Function  : Create a ModBus TCP descriptor
 * rtu_ctl   : configure parameters of ModBus TCP
 * return    : (MB_DESC_T *)=SUCCESS NULL=ERRROR
 */
MB_DESC_T *mb_tcp_init(MBTCP_CTL_T *tcp_ctl);

/*
 * Function  : close a ModBus TCP descriptor
 * mb_desc   : the ModBus TCP descriptor you want to close
 * return    : void
 */
void mb_tcp_close(MB_DESC_T *mb_desc);

/*
 * Function  : recv ModBus TCP data from slaver to ModBus cache
 * mb_desc   : ModBus TCP descriptor
 * mb_data   : ModBus cache
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_tcp_recv(MB_DESC_T *mb_desc, MB_DATA_T *mb_data);

/*
 * Function  : send ModBus TCP data from ModBus cache to slaver
 * mb_desc   : ModBus TCP descriptor
 * mb_data   : ModBus cache
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_tcp_send(MB_DESC_T *mb_desc, MB_DATA_T *mb_data);

#endif
