/*
 * Author   : tanxiaoyang
 * Company  : EVOC
 * Function : 1. Open/Close connection with ModBus TCP server 
 *            2. Recv/Send  data to ModBus TCP server 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h> 
#include <net/if.h>
#include <netinet/tcp.h>

#include "mb_tcp.h"

/*
 * Function : encap the Modbus TCP MBAP(ModBus Application Protocol),will updata cache offset for write
 * mb_data  : ModBus cache
 * return   : void
 */
static void mbap_head_encap(MBTCP_DATA_T *mb_tcp_data)
{
    PTR_CHECK_VOID(mb_tcp_data);
    
    mb_tcp_data->mbap_head[MB_TX].transaction_code++;

    MBAP_HEAD_T mbap_head = mb_tcp_data->mbap_head[MB_TX];
    
    if (!mb_tcp_data->mb_data->is_big_endian)
    {
        mbap_head.transaction_code = b2l_endian(mbap_head.transaction_code);
    }

    *((MBAP_HEAD_T *)mb_tcp_data->mb_data->data) = mbap_head;
    mb_tcp_data->mb_data->data_len += sizeof(MBAP_HEAD_T);
}

/*
 * Function : re-encap the Modbus TCP MBAP(ModBus Application Protocol),will not updata cache offset for write
 * mb_data  : ModBus cache
 * return   : void
 */
static void mbap_head_re_encap(MBTCP_DATA_T *mb_tcp_data)
{
    PTR_CHECK_VOID(mb_tcp_data);

    MB_DATA_T *mb_data  = mb_tcp_data->mb_data;
    UINT16_T   data_len = mb_data->data_len - (sizeof(MBAP_HEAD_T) - 1);

    if (mb_data->is_big_endian)
    {
        data_len = b2l_endian(data_len);
    }

    ((MBAP_HEAD_T *)mb_data->data)->data_length = data_len;
}

/*
 * Function  : decap the Modbus TCP PDU(Protocol Data Unit)
 * mb_data   : ModBus cache
 * mbap_head : ModBus TCP MBAP(ModBus Application Protocol)
 * return    : void
 */
static int mbap_head_decap_check(MBTCP_DATA_T *mb_tcp_data)
{
    PTR_CHECK_N1(mb_tcp_data);

    int ret = 0;
    MB_DATA_T   *mb_data      = mb_tcp_data->mb_data;
    MBAP_HEAD_T *tx_mbap_head = &mb_tcp_data->mbap_head[MB_TX];
    MBAP_HEAD_T  rx_mbap_head = *((MBAP_HEAD_T *)mb_data->data);

    do 
    {
        if (rx_mbap_head.transaction_code != tx_mbap_head->transaction_code)
        {
            printf("Invalid MBAP trasaction code(0x%02x)\n", rx_mbap_head.transaction_code);
            ret = -1;
            break;
        }

        if (rx_mbap_head.protocol_code != tx_mbap_head->protocol_code)
        {
            printf("Invalid MBAP protocol code(0x%02x)\n", rx_mbap_head.protocol_code);
            ret = -1;
            break;
        }

        if (rx_mbap_head.unit_code != tx_mbap_head->unit_code)
        {
            printf("Invalid MBAP unit code(0x%02x)\n", rx_mbap_head.unit_code);
            ret = -1;
            break;
        }
    } while (0);

    mb_tcp_data->mbap_head[MB_RX] = rx_mbap_head;
    mb_data->offset = sizeof(MBAP_HEAD_T);

    return ret;
}

static int tcp_connect(MBTCP_DESC_T *mb_tcp_desc)
{
    PTR_CHECK_N1(mb_tcp_desc);

    int ret  = 0;
    int sock = 0;
    struct sockaddr_in server_addr = {0};
    struct ifreq ifrq = {0};

    /* create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > sock) 
    {
        perror("socket error");
        return -1;
    }

    /* bind tcp_ctl->ethdev */
    snprintf(ifrq.ifr_ifrn.ifrn_name, MBTCP_ETHDEV_LEN, "%s", mb_tcp_desc->ethdev);
    ret = setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifrq, sizeof(ifrq));
    if (ret < 0)
    {
        perror("setsockopt error");
        close(sock);
        return -1;
    }

    /* server ip & server port */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(mb_tcp_desc->port);  
    inet_pton(AF_INET, mb_tcp_desc->ip, &server_addr.sin_addr);

    /* connect server */
    ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        perror("connect error");
        close(sock);
        return -1;
    }

    mb_tcp_desc->socket = sock;

    return 0;
}

static int tcp_re_connect(MBTCP_DESC_T *mb_tcp_desc)
{
    struct tcp_info info;
    int ret    = 0;
    int conntm = 0;
    int len    = sizeof(info);
    int sock   = mb_tcp_desc->socket;

    ret = getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)(&len));
    if (0 > ret)
    {
        perror("getsockopt error");
        return -1;
    }

    if (TCP_ESTABLISHED != info.tcpi_state)
    {
        while (conntm++ <= MBTCP_CONN_TIMEOUT)
        {
            printf("try to re-connect times(%d)\n", conntm);

            close(mb_tcp_desc->socket);

            if (0 > tcp_connect(mb_tcp_desc))
            {
                usleep(MBTCP_CONN_DELAY);
            }
            else
            {
                return 1;
            }
        }
    }
    else
    {
        return 0;
    }

    return -1;
}

static int tcp_recv(MBTCP_DESC_T *mb_tcp_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_tcp_desc);
    PTR_CHECK_N1(mb_data);

    int ret     = 0;
    int ready   = 0;
    int recvtm  = 0;
    int length  = 0;
    int socket  = mb_tcp_desc->socket;

    fd_set rcvset;
    struct timeval tv;

    tv.tv_sec  = 3;
    tv.tv_usec = 0;

    FD_ZERO(&rcvset);
    FD_SET(socket, &rcvset);

    if ((ret = tcp_re_connect(mb_tcp_desc)))
    {
        if (-1 == ret)
        {
            printf("tcp re-connect before recv error\n");
            return -1;
        }
        else
        {
            printf("tcp re-connect before recv success\n");
        }
    }
    
    ready = select(socket + 1, &rcvset, NULL, NULL, &tv);
    if (0 > ready)
    {
        perror("select error");
        return -1;
    }
    else if (0 == ready)
    {
        printf("Select timeout\n");
        return -1;
    }

    mb_data->data_len = 0;

    while ((recvtm++ <= MBTCP_RECV_TIMEOUT))
    {
        length = recv(socket, (mb_data->data + mb_data->data_len), (mb_data->max_data_len - mb_data->data_len), MSG_DONTWAIT);
        if(0 > length)
        {
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
            {
                usleep(MBTCP_RECV_DELAY);
                continue;
            }
            else
            {
                perror("recv error");
                return -1;
            }
        }
        else if(length == 0)
        {
            if ((ret = tcp_re_connect(mb_tcp_desc)))
            {
                if (-1 == ret)
                {
                    printf("tcp re-connect after recv error\n");
                    return -1;
                }
                else
                {
                    printf("tcp re-connect after recv success\n");
                    recvtm = 0;
                    continue;
                }
            }
        }
        else
        {
            mb_data->data_len += length;
            recvtm = 0;
            continue;
        }
    }

    return mb_data->data_len;
}

static int tcp_send(MBTCP_DESC_T *mb_tcp_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_tcp_desc);
    PTR_CHECK_N1(mb_data);

    int ret    = 0;
    int length = 0;
    int socket = mb_tcp_desc->socket;

    /* re-connect */
    if ((ret = tcp_re_connect(mb_tcp_desc)))
    {
        if (-1 == ret)
        {
            printf("tcp re-connect error\n");
            return -1;
        }
        else
        {
            printf("tcp re-connect success\n");
        }
    }

    length = send(socket, mb_data->data, mb_data->data_len, MSG_DONTWAIT);
    if (0 > length)
    {
        perror("write error");
    }

    return length;
}

/*
 * Function  : Create a ModBus TCP context
 * rtu_ctl   : configure parameters of ModBus TCP
 * return    : (MBTCP_CTX_T *)=SUCCESS NULL=ERRROR
 */
MBTCP_CTX_T *mb_tcp_init(MBTCP_CTL_T *tcp_ctl)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_NULL(tcp_ctl);

    MBTCP_CTX_T *mbtcp_ctx = NULL;
    int ret = 0;

    /* create mbtcp context */
    mbtcp_ctx = (MBTCP_CTX_T *)malloc(sizeof(MBTCP_CTX_T));
    if (!mbtcp_ctx)
    {
        printf("Can not create a modbus tcp context\n");
        return NULL;
    }
    memset(mbtcp_ctx, 0, sizeof(MBTCP_CTX_T));

    /* Create mbtcp data */
    mbtcp_ctx->mb_tcp_data.mb_data = mb_data_create(tcp_ctl->max_data_size);
    if (!mbtcp_ctx->mb_tcp_data.mb_data)
    {
        printf("Can not create a modbus tcp data\n");
        free(mbtcp_ctx);
        return NULL;
    }
    mbtcp_ctx->mb_tcp_data.mbap_head[MB_TX].transaction_code = 0x0;
    mbtcp_ctx->mb_tcp_data.mbap_head[MB_TX].protocol_code    = 0x0;
    mbtcp_ctx->mb_tcp_data.mbap_head[MB_TX].unit_code        = tcp_ctl->unitid;

    mbtcp_ctx->mb_tcp_desc.port = tcp_ctl->port;
    snprintf(mbtcp_ctx->mb_tcp_desc.ip, sizeof(mbtcp_ctx->mb_tcp_desc.ip), "%s", tcp_ctl->ip);
    snprintf(mbtcp_ctx->mb_tcp_desc.ethdev, sizeof(mbtcp_ctx->mb_tcp_desc.ethdev), "%s", tcp_ctl->ethdev);

    /* tcp connect */
    ret = tcp_connect(&mbtcp_ctx->mb_tcp_desc);
    if (0 > ret)
    {
        printf("tcp connect failed\n");
        mb_data_destory(mbtcp_ctx->mb_tcp_data.mb_data);
        free(mbtcp_ctx);
        return NULL;
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return mbtcp_ctx;
}

/*
 * Function  : close a ModBus TCP context
 * mbtcp_ctx   : the ModBus TCP context you want to close
 * return    : void
 */
void mb_tcp_close(MBTCP_CTX_T *mbtcp_ctx)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mbtcp_ctx);
    
    close(mbtcp_ctx->mb_tcp_desc.socket);

    mb_data_destory(mbtcp_ctx->mb_tcp_data.mb_data);

    free(mbtcp_ctx);

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

/*
 * Function  : send ModBus TCP data from ModBus cache to slaver
 * mbtcp_ctx : ModBus TCP context
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_tcp_send(MBTCP_CTX_T *mbtcp_ctx)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mbtcp_ctx);

    MBTCP_DATA_T *mbtcp_data = &mbtcp_ctx->mb_tcp_data;
    MB_DATA_T    *mb_data    = mbtcp_ctx->mb_tcp_data.mb_data;
    MBTCP_DESC_T *mbtcp_desc = &(mbtcp_ctx->mb_tcp_desc); 

    mb_data_clear(mb_data);

    /* encap modbus head */
    mbap_head_encap(mbtcp_data);

    /* encap modbus data */
    mb_data_encap(mb_data);

    /* re-encap modbus head */
    mbap_head_re_encap(mbtcp_data);

#ifdef MB_DEBUG
    MB_PRINT("SEND\n");
    mb_cache_show(mb_data);
#endif

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    /* send tcp data */
    return tcp_send(mbtcp_desc, mb_data);
}

/*
 * Function  : recv ModBus TCP data from slaver to ModBus cache
 * mbtcp_ctx   : ModBus TCP context
 * mb_data   : ModBus cache
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_tcp_recv(MBTCP_CTX_T *mbtcp_ctx)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mbtcp_ctx);

    int length = 0;
    MBTCP_DATA_T *mbtcp_data = &mbtcp_ctx->mb_tcp_data;
    MB_DATA_T    *mb_data    = mbtcp_ctx->mb_tcp_data.mb_data;
    MBTCP_DESC_T *mbtcp_desc = &(mbtcp_ctx->mb_tcp_desc); 
    
    mb_data_clear(mb_data);

    /* recv tcp data */
    length = tcp_recv(mbtcp_desc, mb_data);
    if (0 > length)
    {
        return -1;
    }

#ifdef MB_DEBUG
    MB_PRINT("RECV %d bytes\n", length);
    mb_cache_show(mb_data);
#endif

    /* decap modbus head */
    if (0 > mbap_head_decap_check(mbtcp_data))
    {
        return -1;
    }

    /* dacap modbus data */
    mb_data_decap(mb_data);

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return length;
}

void mbtcpctx_info_updata(MBTCP_CTX_T *mbtcp_ctx, MB_INFO_T *mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mbtcp_ctx);
    
    mbtcp_ctx->mb_tcp_data.mb_data->mb_info = *mb_info;

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

void mbtcpctx_info_takeout(MBTCP_CTX_T *mbtcp_ctx, MB_INFO_T *mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mbtcp_ctx);
    PTR_CHECK_VOID(mb_info);
    
    *mb_info = mbtcp_ctx->mb_tcp_data.mb_data->mb_info;

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}
