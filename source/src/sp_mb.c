/*
 * Author   : shawn-tany
 * Function : 1. Open/Close connection with ModBus slaver
 *            2. Recv/Send  data to ModBus slaver
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sp_mb.h"

#define DFT_MBIO_CONFIG_FILE "/usr/local/etc/mb_io.conf"

/*
 * Function : updata ModBus master information to ModBus context
 * mb_ctx   : ModBus context
 * return   : 0=SUCCESS -1=ERROR
 */

static int sp_mbctx_info_updata(SPMB_CTX_T *mb_ctx, MB_INFO_T *mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mb_ctx);
    PTR_CHECK_N1(mb_info);

    int ret = 0;

    do 
    {
        switch (mb_info->code)
        {
            case MB_FUNC_01 : 
            case MB_FUNC_02 : 
            case MB_FUNC_03 :
            case MB_FUNC_04 :
                if (!mb_info->n_reg)
                {
                    ret = -1;
                }
                break;
                
            case MB_FUNC_05 :
            case MB_FUNC_06 : 
                break;
                
            case MB_FUNC_0f : 
            case MB_FUNC_10 : 
                if (!mb_info->n_reg)
                {
                    ret = -1;
                }
                break;
                
            default :
                ret = -1;
        }

        /* failed */
        if (0 > ret)
        {
            break;
        }

        /* updata master info */
        if (MB_TYPE_TCP == mb_ctx->mb_type)
        {
            mbtcpctx_info_updata(mb_ctx->ctx.mb_tcp_ctx, mb_info);
        }
        else if (MB_TYPE_RTU == mb_ctx->mb_type)
        {
            mbrtuctx_info_updata(mb_ctx->ctx.mb_rtu_ctx, mb_info);
        }
        
    } while (0);

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return ret;
}

/*
 * Function : take out ModBus master information from ModBus context
 * mb_ctx   : ModBus context
 * return   : 0=SUCCESS -1=ERROR
 */
static int sp_mbctx_info_takeout(SPMB_CTX_T *mb_ctx, MB_INFO_T *mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mb_ctx);
    PTR_CHECK_N1(mb_info);

    int ret = 0;

    /* take out master info */
    if (MB_TYPE_TCP == mb_ctx->mb_type)
    {
        mbtcpctx_info_takeout(mb_ctx->ctx.mb_tcp_ctx, mb_info);
    }
    else if (MB_TYPE_RTU == mb_ctx->mb_type)
    {
        mbrtuctx_info_takeout(mb_ctx->ctx.mb_rtu_ctx, mb_info);
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return ret;
}

static int sp_io_config(SPMB_CTX_T *mb_ctx, char *filename)
{
    PTR_CHECK_N1(mb_ctx);
    PTR_CHECK_N1(filename);

    FILE *fp         = NULL;
    char  line[512]  = {0};
    char  key[2][16] = {0};
    char *pos        = NULL;
    char *ptr        = NULL;
    int   length     = 0;
    int   i          = 0;

    if (!(fp = fopen(filename, "r")))
    {
        perror("fopen error");
        return -1;
    }

    while (fgets(line, sizeof(line), fp))
    {
        pos = line;

        if (!(length = strlen(pos)))
        {
            continue;
        }

        for (i = 0; i < length; ++i)
        {
            if (pos[i] == ' ' || pos[i] == '\t')
            {
                pos++;
            }
            else
            {
                break;
            }
        }
        length -= i;

        if (!length || '#' == pos[0])
        {
            continue;
        }

        if (pos[length - 1] == '\n')
        {
            pos[length - 1] = 0;
            length -= 1;

            if (length && pos[length - 1] == '\r')
            {
                pos[length - 1] = 0;
                length -= 1;
            }
        }

        if (!length)
        {
            continue;
        }

        ptr = strtok(pos, "=");

        i = 0;
        while (ptr && (i < sizeof(key[0])))
        {
            snprintf(key[i], sizeof(key[i]), "%s", ptr);
            ptr = strtok(NULL, "=");
            i++;
        }

        if (!strcmp(key[0], "IADDR_START"))
        {
            mb_ctx->mb_ioconf.i_addr_start = strtol(key[1], NULL, 0);
        }
        else if (!strcmp(key[0], "OADDR_START"))
        {
            mb_ctx->mb_ioconf.o_addr_start = strtol(key[1], NULL, 0);
        }
        else if (!strcmp(key[0], "I_NUMBER"))
        {
            mb_ctx->mb_ioconf.i_number = strtol(key[1], NULL, 0);
        }
        else if (!strcmp(key[0], "O_NUMBER"))
        {
            mb_ctx->mb_ioconf.o_number = strtol(key[1], NULL, 0);
        }
    }

    printf("IADDR_START = 0x%x\n"
           "I_NUMBER    = %d\n"
           "OADDR_START = 0x%x\n"
           "O_NUMBER    = %d\n",
           mb_ctx->mb_ioconf.i_addr_start, mb_ctx->mb_ioconf.i_number,
           mb_ctx->mb_ioconf.o_addr_start, mb_ctx->mb_ioconf.o_number);

    fclose(fp);

    return 0;
}

/*
 * Function  : Create a ModBus Context
 * mb_ctl    : some information for create ModBus Context
 * return    : (SPMB_CTX_T *)=SUCCESS NULL=ERRROR
 */
SPMB_CTX_T *sp_mb_init(SPMB_CTL_T *mb_ctl)
{
    PTR_CHECK_NULL(mb_ctl);

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    SPMB_CTX_T *mb_ctx = NULL;
    char *ioconf = strlen(mb_ctl->mb_conf) ? mb_ctl->mb_conf : DFT_MBIO_CONFIG_FILE;

    /* create sp modbus context */
    mb_ctx = (SPMB_CTX_T *)malloc(sizeof(SPMB_CTX_T));
    if (!mb_ctx)
    {
        return NULL;
    }
    memset(mb_ctx, 0, sizeof(SPMB_CTX_T));

    /* create a modbus descriptor */
    if (MB_TYPE_TCP == mb_ctl->mb_type)
    {
        mb_ctx->mb_type = MB_TYPE_TCP;

        mb_ctx->ctx.mb_tcp_ctx = mb_tcp_init(&mb_ctl->tcp_ctrl);
        /* check descriptor */
        if (!mb_ctx->ctx.mb_tcp_ctx)
        {
            free(mb_ctx);
            return NULL;
        }
    }
    else if (MB_TYPE_RTU == mb_ctl->mb_type)
    {
        mb_ctx->mb_type = MB_TYPE_RTU;

        mb_ctx->ctx.mb_rtu_ctx = mb_rtu_init(&mb_ctl->rtu_ctrl);
        /* check descriptor */
        if (!mb_ctx->ctx.mb_rtu_ctx)
        {
            free(mb_ctx);
            return NULL;
        }
    }
    else
    {
        printf("Invalid ModBus protocol type(0x%02x)\n", mb_ctl->mb_type);
        free(mb_ctx);
        return NULL;
    }

    sp_io_config(mb_ctx, ioconf);

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return mb_ctx;
}

/*
 * Function : close a ModBus Context
 * mb_ctx   : the ModBus context you want to close
 * return   : void
 */
void sp_mb_close(SPMB_CTX_T *mb_ctx)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mb_ctx);

    /* close modbus descriptor */
    if (MB_TYPE_TCP == mb_ctx->mb_type)
    {
        mb_tcp_close(mb_ctx->ctx.mb_tcp_ctx);
    }
    else if (MB_TYPE_RTU == mb_ctx->mb_type)
    {
        mb_rtu_close(mb_ctx->ctx.mb_rtu_ctx);
    }

    free(mb_ctx);

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

/*
 * Function : recv ModBus data from slaver
 * mb_ctx   : ModBus context
 * return   : 0=CLOSE length=SUCCESS -1=ERROR
 */
int sp_mb_recv(SPMB_CTX_T *mb_ctx, MB_INFO_T *mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mb_ctx);
    PTR_CHECK_N1(mb_info);

    int length = 0;

    /* recv modbus data */
    if (MB_TYPE_TCP == mb_ctx->mb_type)
    {
        length = mb_tcp_recv(mb_ctx->ctx.mb_tcp_ctx);
    }
    else if (MB_TYPE_RTU == mb_ctx->mb_type)
    {
        length = mb_rtu_recv(mb_ctx->ctx.mb_rtu_ctx);
    }

    if (0 > sp_mbctx_info_takeout(mb_ctx, mb_info))
    {
        return -1;
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return length;
}

/*
 * Function : send ModBus data to slaver
 * mb_ctx   : ModBus context
 * return   : 0=CLOSE length=SUCCESS -1=ERROR
 */
int sp_mb_send(SPMB_CTX_T *mb_ctx, MB_INFO_T *mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mb_ctx);

    int length = 0;

    if (0 > sp_mbctx_info_updata(mb_ctx, mb_info))
    {
        return -1;
    }

    /* send modbus data */
    if (MB_TYPE_TCP == mb_ctx->mb_type)
    {
        length = mb_tcp_send(mb_ctx->ctx.mb_tcp_ctx);
    }
    else if (MB_TYPE_RTU == mb_ctx->mb_type)
    {
        length = mb_rtu_send(mb_ctx->ctx.mb_rtu_ctx);
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return length;
}

int sp_mbio_get(SPMB_CTX_T *mb_ctx, IO_DIRECTION_T direction, 
    UINT16_T ioidx, IO_STATUS_T *status)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mb_ctx);
    PTR_CHECK_N1(status);

    UINT16_T coil_num  = (direction == IO_INPUT) ? mb_ctx->mb_ioconf.i_number : 
                                                   mb_ctx->mb_ioconf.o_number;
    UINT16_T func_code = (direction == IO_INPUT) ? MB_FUNC_02 : MB_FUNC_01;
    UINT16_T start_reg = (direction == IO_INPUT) ? mb_ctx->mb_ioconf.i_addr_start : 
                                                   mb_ctx->mb_ioconf.o_addr_start;

    MB_INFO_T get_mb_info = {
        .code  = func_code,
        .reg   = start_reg + ioidx,
        .n_reg = 1
    };

    if (ioidx >= coil_num)
    {
        MB_PRINT("IO GET ERROR : Invalid coil index\n");
        return -1;
    }

    /* send a modbsu request */
    if (0 > sp_mb_send(mb_ctx, &get_mb_info))
    {
        MB_PRINT("IO GET ERROR : ModBus send request failed\n");
        return -1;
    }

    /* recv a modbus response */
    if (0 > sp_mb_recv(mb_ctx, &get_mb_info))
    {
        MB_PRINT("IO GET ERROR : ModBus recv response failed\n");
        return -1;
    }

    *status = *(UINT16_T *)(&get_mb_info.value[0]) ? IO_ON : IO_OFF;
    
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return 0;
}

int sp_mbio_set(SPMB_CTX_T *mb_ctx, UINT16_T ioidx, IO_STATUS_T statu)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mb_ctx);

    MB_INFO_T set_mb_info = {
        .code  = MB_FUNC_05,
        .reg   = ioidx + mb_ctx->mb_ioconf.o_addr_start,
        .n_reg = 1
    };
    *((UINT16_T *)(&set_mb_info.value[0])) = (IO_ON == statu) ? 0xff : 0x00;

    if (ioidx >= mb_ctx->mb_ioconf.o_number)
    {
        MB_PRINT("IO SET ERROR : Invalid coil index\n");
        return -1;
    }

    /* get modbsu output coils request */
    if (0 > sp_mb_send(mb_ctx, &set_mb_info))
    {
        MB_PRINT("IO SET ERROR : ModBus send request failed\n");
        return -1;
    }

    /* get modbsu output coils response */
    if (0 > sp_mb_recv(mb_ctx, &set_mb_info))
    {
        MB_PRINT("IO SET ERROR : ModBus recv response failed\n");
        return -1;
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return 0;
}

/*
 * Function  : show response status from ModBus slaver
 * mb_info   : ModBus master info
 * return    : void
 */
void sp_mb_status_show(MB_INFO_T mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    if (!(mb_info.err))
    {
        printf("MB SUCCESS\n");
        return ;
    }

    UINT8_T code = mb_info.code & (~0x80);
    int i = 0;

    printf("MB ERROR CODE(0x%02x) : ", mb_info.err);

    switch (mb_info.err)
    {
        case MB_ERR_FUNC :
            printf("Invalid function code(0x%02x)\n", code);
            break;

        case MB_ERR_ADDR :
            printf("Invalid register address(0x%02x)\n", mb_info.reg);
            break;

        case MB_ERR_DATA :
            printf("Invalid data\n");
            for (i = 0; i < mb_info.n_byte; ++i)
            {
                printf("Data[%d] : 0x%02x\n", i, mb_info.value[i]);
            }
            break;

        case MB_ERR_SERVER :
            printf("Server failure\n");
            break;

        case MB_ERR_WAIT :
            printf("The request has been confirmed, but not processed completely\n");
            break;

        case MB_ERR_BUSY :
            printf("Server busy\n");
            break;

        case MB_ERR_UNREACHABLE :
            printf("Network target unreachable\n");
            break;

        case MB_ERR_UNRESPONSIVE :
            printf("Network target unresponsive\n");
            break;
    
        default :
            printf("Unkown error\n");
            break;
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

/*
 * Function  : show ModBus data after decap
 * mb_info   : ModBus master info
 * return    : void
 */
void sp_mb_data_show(MB_INFO_T mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    int i = 0;

    if (mb_info.err)
    {
        return ;
    }

    switch (mb_info.code)
    {
        case MB_FUNC_01 : 
        case MB_FUNC_02 : 
        case MB_FUNC_03 : 
        case MB_FUNC_04 : 
            printf("mb_info.code = 0x%02x\n", mb_info.code);
            printf("mb_info.n_byte = %d\n", mb_info.n_byte);
            /* value */
            for (i = 0; i < mb_info.n_byte; i++)
            {
                printf("mb_info.value[%d] = 0x%02x\n", i + 1, mb_info.value[i]);
            }
            break;
            
        case MB_FUNC_05 : 
        case MB_FUNC_06 : 
            printf("mb_info.code = 0x%02x\n", mb_info.code);
            printf("mb_info.reg = 0x%04x\n", mb_info.reg);
            printf("mb_info.value = 0x%04x\n", *(UINT16_T *)(&mb_info.value[0]));
            break;
            
        case MB_FUNC_0f : 
        case MB_FUNC_10 : 
            printf("mb_info.code = 0x%02x\n", mb_info.code);
            printf("mb_info.reg = 0x%04x\n", mb_info.reg);
            printf("mb_info.n_reg = 0x%04x\n", mb_info.n_reg);
            break;
            
        default :
            printf("Unkown function code 0x%02x\n", mb_info.code);
            break;
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}