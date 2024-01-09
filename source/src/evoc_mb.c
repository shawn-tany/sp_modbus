/*
 * Author   : tanxiaoyang
 * Company  : EVOC
 * Function : 1. Open/Close connection with ModBus slaver
 *            2. Recv/Send  data to ModBus slaver
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "evoc_mb.h"

/*
 * Function : updata ModBus master information to ModBus context
 * mb_ctx   : ModBus context
 * return   : 0=SUCCESS -1=ERROR
 */

static int evoc_mbctx_info_updata(EVOCMB_CTX_T *mb_ctx, MB_INFO_T *mb_info)
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
static int evoc_mbctx_info_takeout(EVOCMB_CTX_T *mb_ctx, MB_INFO_T *mb_info)
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

    if (mb_info->err)
    {
        ret = -1;
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return ret;
}

/*
 * Function  : Create a ModBus Context
 * mb_ctl    : some information for create modBus Context
 * return    : (EVOCMB_CTX_T *)=SUCCESS NULL=ERRROR
 */
EVOCMB_CTX_T *evoc_mb_init(EVOCMB_CTL_T *mb_ctl)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    EVOCMB_CTX_T *mb_ctx = NULL;

    /* create evoc modbus context */
    mb_ctx = (EVOCMB_CTX_T *)malloc(sizeof(EVOCMB_CTX_T));
    if (!mb_ctx)
    {
        return NULL;
    }
    memset(mb_ctx, 0, sizeof(EVOCMB_CTX_T));

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

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return mb_ctx;
}

/*
 * Function : close a ModBus Context
 * mb_ctx   : the ModBus context you want to close
 * return   : void
 */
void evoc_mb_close(EVOCMB_CTX_T *mb_ctx)
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
int evoc_mb_recv(EVOCMB_CTX_T *mb_ctx, MB_INFO_T *mb_info)
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

    if (0 > evoc_mbctx_info_takeout(mb_ctx, mb_info))
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
int evoc_mb_send(EVOCMB_CTX_T *mb_ctx, MB_INFO_T *mb_info)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_N1(mb_ctx);

    int length = 0;

    if (0 > evoc_mbctx_info_updata(mb_ctx, mb_info))
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

/*
 * Function  : show response status from ModBus slaver
 * mb_info   : ModBus master info
 * return    : void
 */
void evoc_mb_status_show(MB_INFO_T mb_info)
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
void evoc_mb_data_show(MB_INFO_T mb_info)
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