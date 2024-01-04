#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include "mb_rtu.h"

static void mbrtu_slaveaddr_encap(MBRTU_DATA_T *mb_rtu_data)
{
    PTR_CHECK_VOID(mb_rtu_data);

    MB_DATA_T *mb_data = mb_rtu_data->mb_data;

    MBDATA_BYTE_SET(mb_data, mb_rtu_data->rtu_info[MB_TX].slaver_addr);
}

static int mbrtu_slaveaddr_decap_check(MBRTU_DATA_T *mb_rtu_data)
{
    PTR_CHECK_N1(mb_rtu_data);

    MB_DATA_T    *mb_data     = mb_rtu_data->mb_data;
    MBRTU_INFO_T *rx_rtu_info = &mb_rtu_data->rtu_info[MB_RX];
    MBRTU_INFO_T *tx_rtu_info = &mb_rtu_data->rtu_info[MB_TX];

    MBDATA_BYTE_GET(mb_data, rx_rtu_info->slaver_addr);

    if (rx_rtu_info->slaver_addr != tx_rtu_info->slaver_addr)
    {
        printf("Invalid slaver address(0x%02x)\n", rx_rtu_info->slaver_addr);
        return -1;
    }

    return 0;
}

/*
 * Function  : calculate CRC checksum
 * data      : data used to calculate checksum
 * mb_data   : data length used to calculate checksum
 * return    : checksum
 */
static UINT16_T crc_calc(UINT8_T *data, int data_len)
{
    int i   = 0;
    int j   = 0;
    int crc = 0xffff;

    /* every byte */
    for (i = 0; i < data_len; ++i)
    {
        crc ^= data[i];

        /* every bit */
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc = (crc >> 1);
            }
        }
    }

    return crc;
}

static void mbrtu_crc_encap(MBRTU_DATA_T *mb_rtu_data)
{
    PTR_CHECK_VOID(mb_rtu_data);

    MB_DATA_T *mb_data = mb_rtu_data->mb_data;
    UINT16_T crc = crc_calc(mb_data->data, mb_data->data_len);

    if (!mb_data->is_big_endian)
    {
        crc = b2l_endian(crc);
    }

    MBDATA_WORD_SET(mb_data, crc);

    mb_rtu_data->rtu_info[MB_TX].crc_checksum = crc;
}

static int mbrtu_crc_decap_check(MBRTU_DATA_T *mb_rtu_data)
{
    PTR_CHECK_N1(mb_rtu_data);

    MB_DATA_T *mb_data = mb_rtu_data->mb_data;
    UINT16_T crc = 0;

    MBDATA_WORD_GET(mb_data, crc);

    if (!mb_data->is_big_endian)
    {
        crc = b2l_endian(crc);
    }

    mb_rtu_data->rtu_info[MB_RX].crc_checksum = crc;
    mb_data->data_len -= sizeof(UINT16_T);

    crc = crc_calc(mb_data->data, mb_data->data_len);

    if (crc != mb_rtu_data->rtu_info[MB_RX].crc_checksum)
    {
        printf("Invalid CRC checksum(0x%02x), should be 0x%02x\n", mb_rtu_data->rtu_info[MB_RX].crc_checksum, crc);
        return -1;
    }

    return 0;
}

static int com_recv(MBRTU_DESC_T *mb_rtu_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_rtu_desc);
    PTR_CHECK_N1(mb_data);

    int length  = 0;
    int timeout = 0;
    int comfd   = mb_rtu_desc->com_fd;
    int ready   = 0;

    fd_set readset;
    struct timeval tv;

    tv.tv_sec  = 3;
    tv.tv_usec = 0;

    FD_ZERO(&readset);
    FD_SET(comfd, &readset);

    ready = select(comfd + 1, &readset, NULL, NULL, &tv);
    if (0 > ready)
    {
        perror("select error");
        return -1;
    }
    else if (0 == ready)
    {
        printf("select timeout\n");
        return -1;
    }

    mb_data->data_len = 0;

    while ((timeout++ <= MBRTU_RECV_TIMEOUT))
    {
        length = read(comfd, (mb_data->data + mb_data->data_len), (mb_data->max_data_len - mb_data->data_len));
        if(0 > length)
        {
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
            {
                usleep(MBRTU_RECV_DELAY);
            }
            else
            {
                perror("read error");
                return -1;
            }
        }
        else if(length == 0)
        {
            usleep(MBRTU_RECV_DELAY);
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

static int com_send(MBRTU_DESC_T *mb_rtu_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_rtu_desc);
    PTR_CHECK_N1(mb_data);

    int length  = 0;
    int comfd   = mb_rtu_desc->com_fd;

    length = write(comfd, mb_data->data, mb_data->data_len);
    if (0 > length)
    {
        perror("write error");
        
        /* clear file cache */
        tcflush(comfd, TCIFLUSH);
    }

    return length;
}

static UINT32_T baudrate_convert(UINT32_T baudrate)
{
    struct baudrate_map
    {
        UINT32_T usr_baudrate;
        UINT32_T sys_baudrate;
    } baudrate_list[] = {
        { 50,     B50     },
        { 75,     B75     },
        { 110,    B110    },
        { 134,    B134    },
        { 150,    B150    },
        { 200,    B200    },
        { 300,    B300    },
        { 600,    B600    },
        { 1200,   B1200   },
        { 1800,   B1800   },
        { 2400,   B2400   },
        { 4800,   B4800   },
        { 9600,   B9600   },
        { 19200,  B19200  },
        { 38400,  B38400  },
        { 57600,  B57600  },
        { 115200, B115200 },
        { 921600, B921600 }
    };

    int i = 0;

    for (i = 0; i < ITEM(baudrate_list); ++i)
    {
        if (baudrate == baudrate_list[i].usr_baudrate)
        {
            return baudrate_list[i].sys_baudrate;
        }
    }

    return B9600;
}

/*
 * Function  : configure serial port
 * fd        : file discriptor for serial communication
 * rtu_ctl   : configure parameters of ModBus RTU
 * return    : 0=SUCCESS -1=ERROR
 */
static int com_config(int fd, MBRTU_CTL_T *rtu_ctl)
{
    PTR_CHECK_N1(rtu_ctl);

    int read_cache_size = 4096;
    struct termios option;

    /* termios original mode */
    memset(&option, 0, sizeof(option));
    cfmakeraw(&option);

    /* get current termios configure */
    if (0 != tcgetattr(fd, &option))
    {
        perror("tcgetattr error");
        return -1;
    }

    /* baudrate configure */
    rtu_ctl->baudrate = baudrate_convert(rtu_ctl->baudrate);
    if (0 != cfsetispeed(&option, rtu_ctl->baudrate) ||
        0 != cfsetospeed(&option, rtu_ctl->baudrate))
    {
        perror("failed to set buadrate");
        return -1;
    }
    option.c_cflag |= CLOCAL;
    option.c_cflag |= CREAD;

    /* data bit configure */
    option.c_cflag &= ~CSIZE;
    switch (rtu_ctl->databit)
    {
        case 5 :
            option.c_cflag |= CS5;
            break;

        case 6 :
            option.c_cflag |= CS6;
            break;

        case 7 :
            option.c_cflag |= CS7;
            break;

        default :
            option.c_cflag |= CS8;
            break;
    }

    /* stop bit configure */
    switch (rtu_ctl->stopbit)
    {
        case 2 :
            option.c_cflag |= CSTOPB;
            break;

        default :
            option.c_cflag &= ~(CSTOPB);
    }

    /* parity configure */
    option.c_cflag &= ~(PARENB | PARODD);
    switch (rtu_ctl->parity)
    {
        case 0 :
            option.c_iflag &= ~(INPCK);
            break;

        case 1 :
            option.c_cflag |= (PARENB | PARODD);
            option.c_iflag |= INPCK;
            break;

        case 2 :
            option.c_cflag |= PARENB;
            option.c_iflag |= INPCK;
            break;
    }
    
    /* flow control */
    switch (rtu_ctl->flowctl)
    {
        case 0 :
            option.c_cflag &= ~(IXON | IXOFF | IXANY);
            option.c_cflag &= ~(CRTSCTS);
            break;

        case 1 :
            option.c_cflag |= (CRTSCTS);
            break;

        case 2 :
            option.c_cflag |= (IXON | IXOFF | IXANY);
    }

    /* raw mode */
    option.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    option.c_oflag &= ~(OPOST);

    /* timeout & min byte */
    option.c_cc[VTIME] = 5;
    option.c_cc[VMIN ] = 0;

    /* read cache */
    if (0 > ioctl(fd, FIONREAD, &read_cache_size))
    {
        printf("failed to set cache size of read\n");
        return -1;
    }

    /* termios configure updata */
    if (0 != tcsetattr(fd, TCSANOW, &option))
    {
        perror("tcsetattr error");
        return -1;
    }

    return 0;
}

/*
 * Function  : Create a ModBus RTU context
 * rtu_ctl   : configure parameters of ModBus RTU
 * return    : (MBRTU_CTX_T *)=SUCCESS NULL=ERRROR
 */
MBRTU_CTX_T *mb_rtu_init(MBRTU_CTL_T *rtu_ctl)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_NULL(rtu_ctl);

    MBRTU_CTX_T *mbrtu_ctx = NULL;
    int fd   = -1;
    int flag = 0;

    /* Create a ModBus RTU context */
    mbrtu_ctx = (MBRTU_CTX_T *)malloc(sizeof(MBRTU_CTX_T));
    if (!mbrtu_ctx)
    {
        printf("Can not Create a ModBus RTU context\n");
        return NULL;
    }
    memset(mbrtu_ctx, 0, sizeof(*mbrtu_ctx));

    mbrtu_ctx->mb_rtu_data.rtu_info[MB_TX].slaver_addr = rtu_ctl->slaver_addr;

    /* Create a ModBus RTU data */
    mbrtu_ctx->mb_rtu_data.mb_data = mb_data_create(rtu_ctl->max_data_size);
    if (!mbrtu_ctx->mb_rtu_data.mb_data)
    {
        printf("Can not Create a ModBus RTU data\n");
        free(mbrtu_ctx);
        return NULL;
    }

    /* Open serial port */
    fd = open(rtu_ctl->serial, O_RDWR | O_NOCTTY);
    if (0 > fd)
    {
        perror("open error");
        mb_data_destory(mbrtu_ctx->mb_rtu_data.mb_data);
        free(mbrtu_ctx);
        return NULL;
    }
    /* no block */
    flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);

    /* com configure */
    if (com_config(fd, rtu_ctl))
    {
        printf("com configure error\n");
        mb_data_destory(mbrtu_ctx->mb_rtu_data.mb_data);
        free(mbrtu_ctx);
        close(fd);
        return NULL;
    }

    /* clear file cache */
    tcflush(fd, TCIFLUSH);

    mbrtu_ctx->mb_rtu_desc.com_fd = fd;

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return mbrtu_ctx;
}

/*
 * Function  : close a ModBus RTU context
 * mbrtu_ctx : the ModBus RTU context you want to close
 * return    : void
 */
void mb_rtu_close(MBRTU_CTX_T *mbrtu_ctx)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mbrtu_ctx);

    close(mbrtu_ctx->mb_rtu_desc.com_fd);

    mb_data_destory(mbrtu_ctx->mb_rtu_data.mb_data);

    free(mbrtu_ctx);

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

/*
 * Function  : send ModBus RTU data from ModBus cache to slaver
 * mbrtu_ctx : ModBus RTU context
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_rtu_send(MBRTU_CTX_T *mbrtu_ctx)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mbrtu_ctx);

    MBRTU_DATA_T *mb_rtu_data = &mbrtu_ctx->mb_rtu_data;
    MB_DATA_T    *mb_data     = mbrtu_ctx->mb_rtu_data.mb_data;

    /* clear data cache */
    mb_data_clear(mb_data);

    /* encap slaver address */
    mbrtu_slaveaddr_encap(mb_rtu_data);

    /* encap PDU */
    mb_data_encap(mb_data);

    /* encap CRC */
    mbrtu_crc_encap(mb_rtu_data);

#ifdef MB_DEBUG
    MB_PRINT("SEND\n");
    mb_cache_show(mb_data);
#endif

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return com_send(&mbrtu_ctx->mb_rtu_desc, mb_data);
}

/*
 * Function  : recv ModBus RTU data from slaver to ModBus cache
 * mbrtu_ctx : ModBus RTU context
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_rtu_recv(MBRTU_CTX_T *mbrtu_ctx)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mbrtu_ctx);

    int length = 0;
    MBRTU_DATA_T *mb_rtu_data = &mbrtu_ctx->mb_rtu_data;
    MB_DATA_T    *mb_data     = mbrtu_ctx->mb_rtu_data.mb_data;

    /* clear data cache */
    mb_data_clear(mb_data);

    /* recv data from ModBus slaver */
    length = com_recv(&mbrtu_ctx->mb_rtu_desc, mb_data);
    if (0 > length)
    {
        return -1;
    }

#ifdef MB_DEBUG
    MB_PRINT("RECV %d bytes\n", length);
    mb_cache_show(mb_data);
#endif

    /* decap slaver address */
    if (0 > mbrtu_slaveaddr_decap_check(mb_rtu_data))
    {
        return -1;
    }

    /* decap PDU */
    mb_data_decap(mb_data);

    /* decap CRC */
    if (mbrtu_crc_decap_check(mb_rtu_data))
    {
        return -1;
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return length;
}

void mbrtuctx_info_updata(MBRTU_CTX_T *mbrtu_ctx, MB_INFO_T *mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mbrtu_ctx);
    PTR_CHECK_VOID(mb_info);
    
    mbrtu_ctx->mb_rtu_data.mb_data->mb_info = *mb_info;

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

void mbrtuctx_info_takeout(MBRTU_CTX_T *mbrtu_ctx, MB_INFO_T *mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mbrtu_ctx);
    PTR_CHECK_VOID(mb_info);
    
    *mb_info = mbrtu_ctx->mb_rtu_data.mb_data->mb_info;

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}
