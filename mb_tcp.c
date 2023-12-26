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
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h> 
#include <net/if.h>

#include "mb_tcp.h"

/*
 * Function : encap the Modbus TCP MBAP(ModBus Application Protocol),will updata cache offset for write
 * mb_data  : ModBus cache
 * return   : void
 */
static void mbap_head_encap(MB_DATA_T *mb_data)
{
    PTR_CHECK_VOID(mb_data);

    MBAP_HEAD_T mbap_head = {
        .protocol_code.W1 = 0x0,
        .data_length.W1   = 0x0,
        .unit_code        = 0x1
    };
    
    UINT16_T transaction_code = ((MBAP_HEAD_T *)mb_data->data)->transaction_code.W1;
    
    if (!mb_data->is_big_endian)
    {
        transaction_code = b2l_endian(transaction_code);
    }
    
    mbap_head.transaction_code.W1 = ++transaction_code;

    *((MBAP_HEAD_T *)mb_data->data) = mbap_head;
    mb_data->data_len += sizeof(MBAP_HEAD_T);
}

/*
 * Function : re-encap the Modbus TCP MBAP(ModBus Application Protocol),will not updata cache offset for write
 * mb_data  : ModBus cache
 * return   : void
 */
static void mbap_head_re_encap(MB_DATA_T *mb_data)
{
    PTR_CHECK_VOID(mb_data);

    UINT16_T data_len = mb_data->data_len - (sizeof(MBAP_HEAD_T) - 1);

    if (mb_data->is_big_endian)
    {
        data_len = b2l_endian(data_len);
    }

    ((MBAP_HEAD_T *)mb_data->data)->data_length.W1 = data_len;
}

/*
 * Function  : decap the Modbus TCP PDU(Protocol Data Unit)
 * mb_data   : ModBus cache
 * mbap_head : ModBus TCP MBAP(ModBus Application Protocol)
 * return    : void
 */
static void mbap_head_decap(MB_DATA_T *mb_data, MBAP_HEAD_T *mbap_head)
{
    PTR_CHECK_VOID(mb_data);
    PTR_CHECK_VOID(mbap_head);

    *mbap_head = *((MBAP_HEAD_T *)mb_data->data);
    
    mb_data->offset = sizeof(MBAP_HEAD_T);
}

static int tcp_recv(MB_DESC_T *mb_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_desc);
    PTR_CHECK_N1(mb_data);

    int ready   = 0;
    int timeout = 0;
    int length  = 0;
    int socket  = mb_desc->mb_tcp_desc.socket;

    fd_set rcvset;
    struct timeval tv;

    tv.tv_sec  = 3;
    tv.tv_usec = 0;

    FD_ZERO(&rcvset);
    FD_SET(socket, &rcvset);
    
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

    while ((timeout++ <= MBTCP_RECV_TIMEOUT))
    {
        length = recv(socket, (mb_data->data + length), (mb_data->max_data_len - length), MSG_DONTWAIT);
        if(0 > length)
        {
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
            {
                usleep(MBTCP_RECV_DELAY);
            }
            else
            {
                perror("recv error");
                return -1;
            }
        }
        else if(length == 0)
        {
            return 0;
        }
        else
        {
            mb_data->data_len += length;
            timeout = 0;
            continue;
        }
    }

    return mb_data->data_len;
}

static int tcp_send(MB_DESC_T *mb_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_desc);
    PTR_CHECK_N1(mb_data);

    int length = 0;
    int socket = mb_desc->mb_tcp_desc.socket;

    length = send(socket, mb_data->data, mb_data->data_len, MSG_DONTWAIT);
    if (0 > length)
    {
        perror("write error");
    }

    return length;
}

/*
 * Function  : Create a ModBus TCP descriptor
 * rtu_ctl   : configure parameters of ModBus TCP
 * return    : (MB_DESC_T *)=SUCCESS NULL=ERRROR
 */
MB_DESC_T *mb_tcp_init(MBTCP_CTL_T *tcp_ctl)
{
    PTR_CHECK_NULL(tcp_ctl);

    MB_DESC_T *mb_desc = NULL;
    int ret;
    int sock = 0;
    struct sockaddr_in server_addr = {0};
    struct ifreq ifrq = {0};

    /* create mbtcp descriptor */
    mb_desc = (MB_DESC_T *)malloc(sizeof(MB_DESC_T));
    if (!mb_desc)
    {
        printf("Can not create a modbus tcp descriptor\n");
        return NULL;
    }
    memset(mb_desc, 0, sizeof(MB_DESC_T));

    /* create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > mb_desc->mb_tcp_desc.socket) 
    {
        perror("socket error");
        free(mb_desc);
        return NULL;
    }

    /* bind tcp_ctl->ethdev */
    snprintf(ifrq.ifr_ifrn.ifrn_name, MBTCP_ETHDEV_LEN, "%s", tcp_ctl->ethdev);
    ret = setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifrq, sizeof(ifrq));
    if (ret < 0)
    {
        perror("setsockopt error");
        free(mb_desc);
        close(sock);
        return NULL;
    }

    /* server ip & server port */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(tcp_ctl->port);  
    inet_pton(AF_INET, tcp_ctl->ip, &server_addr.sin_addr);

    /* connect server */
    ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        perror("connect error");
        free(mb_desc);
        close(sock);
        return NULL;
    }

    mb_desc->mb_tcp_desc.socket = sock;

    return mb_desc;
}

/*
 * Function  : close a ModBus TCP descriptor
 * mb_desc   : the ModBus TCP descriptor you want to close
 * return    : void
 */
void mb_tcp_close(MB_DESC_T *mb_desc)
{
    PTR_CHECK_VOID(mb_desc);
    
    close(mb_desc->mb_tcp_desc.socket);

    free(mb_desc);
}

/*
 * Function  : send ModBus TCP data from ModBus cache to slaver
 * mb_desc   : ModBus TCP descriptor
 * mb_data   : ModBus cache
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_tcp_send(MB_DESC_T *mb_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_desc);
    PTR_CHECK_N1(mb_data);
    
    mb_data_clear(mb_data);

    /* encap modbus head */
    mbap_head_encap(mb_data);

    /* encap modbus data */
    mb_data_encap(mb_data);

    /* re-encap modbus head */
    mbap_head_re_encap(mb_data);

#ifdef MB_DEBUG
    MB_PRINT("SEND\n");
    mb_cache_show(mb_data);
#endif

    /* send tcp data */
    return tcp_send(mb_desc, mb_data);
}

/*
 * Function  : recv ModBus TCP data from slaver to ModBus cache
 * mb_desc   : ModBus TCP descriptor
 * mb_data   : ModBus cache
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_tcp_recv(MB_DESC_T *mb_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_desc);
    PTR_CHECK_N1(mb_data);

    int length = 0;
    MBAP_HEAD_T mbap_head;
    
    mb_data_clear(mb_data);

    /* recv tcp data */
    length = tcp_recv(mb_desc, mb_data);
    if (0 > length)
    {
        return -1;
    }

#ifdef MB_DEBUG
    MB_PRINT("RECV\n");
    mb_cache_show(mb_data);
#endif

    /* decap modbus head */
    mbap_head_decap(mb_data, &mbap_head);

    /* dacap modbus data */
    mb_data_decap(mb_data);

    return length;
}
