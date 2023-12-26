#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>

#include "mb_rtu.h"

static void mbrtu_slaveaddr_encap(MB_DATA_T *mb_data)
{
    PTR_CHECK_VOID(mb_data);

    MBDATA_BYTE_SET(mb_data, mb_data->slave_addr);
}

static void mbrtu_slaveaddr_decap(MB_DATA_T *mb_data)
{
    PTR_CHECK_VOID(mb_data);

    MBDATA_BYTE_GET(mb_data, mb_data->slave_addr);
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

static void mbrtu_crc_encap(MB_DATA_T *mb_data)
{
    PTR_CHECK_VOID(mb_data);

    UINT16_T crc = crc_calc(mb_data->data, mb_data->data_len);

    if (!mb_data->is_big_endian)
    {
        crc = b2l_endian(crc);
    }

    MBDATA_WORD_SET(mb_data, crc);

    mb_data->master_info.checksum = crc;
}

static void mbrtu_crc_decap(MB_DATA_T *mb_data)
{
    PTR_CHECK_VOID(mb_data);

    UINT16_T crc = 0;

    MBDATA_BYTE_GET(mb_data, crc);

    if (mb_data->is_big_endian)
    {
        crc = b2l_endian(crc);
    }
    
    mb_data->master_info.checksum = crc;
}

static int com_recv(MB_DESC_T *mb_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_desc);
    PTR_CHECK_N1(mb_data);

    int length  = 0;
    int timeout = 0;
    int comfd   = mb_desc->mb_rtu_desc.com_fd;
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
        length = read(comfd, (mb_data->data + length), (mb_data->max_data_len - length));
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

static int com_send(MB_DESC_T *mb_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_desc);
    PTR_CHECK_N1(mb_data);

    int length  = 0;
    int comfd   = mb_desc->mb_rtu_desc.com_fd;

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

    struct termios option;

    /* termios original mode */
    memset(&option, 0, sizeof(option));
    cfmakeraw(&option);

    /* get current termios configure */
    tcgetattr(fd, &option);

    /* baudrate configure */
    rtu_ctl->baudrate = baudrate_convert(rtu_ctl->baudrate);
    cfsetispeed(&option, rtu_ctl->baudrate);
    cfsetospeed(&option, rtu_ctl->baudrate);
    option.c_cflag |= CLOCAL;
    option.c_cflag |= CREAD;

    /* flow control */
    switch (rtu_ctl->flowctl)
    {
        case 0 :
            option.c_cflag &= ~(CRTSCTS);
            break;

        case 1 :
            option.c_cflag |= ~(CRTSCTS);
            break;

        case 2 :
            option.c_cflag |= (IXON | IXOFF | IXANY);
    }

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

    /* parity configure */
    switch (rtu_ctl->parity)
    {
        case 0 :
            option.c_cflag &= ~(PARENB);
            break;

        case 1 :
            option.c_cflag |= PARENB;
            option.c_cflag &= ~(PARODD);
            break;

        case 2 :
            option.c_cflag |= PARENB;
            option.c_cflag |= PARODD;
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
    
    /* other termios configure */
    option.c_iflag &= ~(INPCK | ISTRIP);
    option.c_oflag &= ~(OPOST);
    option.c_cc[VTIME] = 0;
    option.c_cc[VMIN ] = 0;

    /* termios configure updata */
    tcsetattr(fd, TCSANOW, &option);

    return 0;
}

/*
 * Function  : Create a ModBus RTU descriptor
 * rtu_ctl   : configure parameters of ModBus RTU
 * return    : (MB_DESC_T *)=SUCCESS NULL=ERRROR
 */
MB_DESC_T *mb_rtu_init(MBRTU_CTL_T *rtu_ctl)
{
    PTR_CHECK_NULL(rtu_ctl);

    MB_DESC_T *mb_desc = NULL;
    int fd   = -1;
    int flag = 0;

    /* Create a ModBus RTU descriptor */
    mb_desc = (MB_DESC_T *)malloc(sizeof(MB_DESC_T));
    if (!mb_desc)
    {
        printf("Can not Create a ModBus RTU descriptor\n");
        return NULL;
    }
    memset(mb_desc, 0, sizeof(*mb_desc));

    /* Open serial port */
    fd = open(rtu_ctl->serial, O_RDWR | O_NOCTTY);
    if (0 > fd)
    {
        perror("open error");
        free(mb_desc);
        return NULL;
    }
    /* no block */
    flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);

    /* com configure */
    if (com_config(fd, rtu_ctl))
    {
        printf("com configure error\n");
        free(mb_desc);
        close(fd);
        return NULL;
    }

    /* clear file cache */
    tcflush(fd, TCIFLUSH);

    mb_desc->mb_rtu_desc.com_fd = fd;

    return mb_desc;
}

/*
 * Function  : close a ModBus RTU descriptor
 * mb_desc   : the ModBus RTU descriptor you want to close
 * return    : void
 */
void mb_rtu_close(MB_DESC_T *mb_desc)
{
    PTR_CHECK_VOID(mb_desc);

    close(mb_desc->mb_rtu_desc.com_fd);

    free(mb_desc);
}

/*
 * Function  : send ModBus RTU data from ModBus cache to slaver
 * mb_desc   : ModBus RTU descriptor
 * mb_data   : ModBus cache
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_rtu_send(MB_DESC_T *mb_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_desc);
    PTR_CHECK_N1(mb_data);

    /* clear data cache */
    mb_data_clear(mb_data);

    /* encap slaver address */
    mbrtu_slaveaddr_encap(mb_data);

    /* encap PDU */
    mb_data_encap(mb_data);

    /* encap CRC */
    mbrtu_crc_encap(mb_data);

#ifdef MB_DEBUG
    MB_PRINT("SEND\n");
    mb_cache_print(mb_data);
#endif

    return com_send(mb_desc, mb_data);
}

/*
 * Function  : recv ModBus RTU data from slaver to ModBus cache
 * mb_desc   : ModBus RTU descriptor
 * mb_data   : ModBus cache
 * return    : 0=CLOSE length=SUCCESS -1=ERROR
 */
int mb_rtu_recv(MB_DESC_T *mb_desc, MB_DATA_T *mb_data)
{
    PTR_CHECK_N1(mb_desc);
    PTR_CHECK_N1(mb_data);

    int length = 0;

    /* clear data cache */
    mb_data_clear(mb_data);

    /* recv data from ModBus slaver */
    length = com_recv(mb_desc, mb_data);
    if (0 > length)
    {
        return -1;
    }

#ifdef MB_DEBUG
    MB_PRINT("RECV\n");
    mb_cache_print(mb_data);
#endif

    /* decap slaver address */
    mbrtu_slaveaddr_decap(mb_data);

    /* decap PDU */
    mb_data_decap(mb_data);

    /* decap CRC */
    mbrtu_crc_decap(mb_data);

    return length;
}
