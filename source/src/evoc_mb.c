/*
 * Author   : tanxiaoyang
 * Company  : EVOC
 * Function : 1. Open/Close connection with ModBus slaver
 *            2. Recv/Send  data to ModBus slaver
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "evoc_mb.h"

#define LOCK(lock)  pthread_mutex_lock(lock)
#define ULOCK(lock) pthread_mutex_unlock(lock)

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

static void *stay_connected_routine(void *arg)
{
    PTR_CHECK_NULL(arg);

    pthread_detach(pthread_self());

    UINT8_T stay = 0;
    EVOCMB_CTX_T *mb_ctx = (EVOCMB_CTX_T *)arg;

    MB_INFO_T mb_info = {
        .code  = MB_FUNC_01,
        .reg   = 0x0000,
        .n_reg = 16
    };

    while (1)
    {
        sleep(1);

        evoc_mb_stay_get(mb_ctx, &stay);

        if (!stay)
        {
            continue;
        }

        /* send a modbsu request */
        if (0 > evoc_mb_send(mb_ctx, &mb_info))
        {
            MB_PRINT("THREAD ERROR : ModBus send request failed\n");
            continue;
        }

        /* recv a modbus response */
        if (0 > evoc_mb_recv(mb_ctx, &mb_info))
        {
            MB_PRINT("THREAD ERROR : ModBus recv response failed\n");
            continue;
        }
    }

    return NULL;
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

    /* ModBus stay connected */
    if (0 > pthread_create(&(mb_ctx->mb_resc.pid), NULL, stay_connected_routine, mb_ctx))
    {
        perror("thread create error");

        if (MB_TYPE_TCP == mb_ctl->mb_type)
        {
            mb_tcp_close(mb_ctx->ctx.mb_tcp_ctx);
        }
        else
        {
            mb_rtu_close(mb_ctx->ctx.mb_rtu_ctx);
        }

        free(mb_ctx);
        return NULL;
    }

    /* Create mutex lock */
    pthread_mutex_init(&(mb_ctx->mb_resc.lock), NULL);

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

    LOCK(&(mb_ctx->mb_resc.lock));

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

    ULOCK(&(mb_ctx->mb_resc.lock));

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

    LOCK(&(mb_ctx->mb_resc.lock));

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

    ULOCK(&(mb_ctx->mb_resc.lock));

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

    LOCK(&(mb_ctx->mb_resc.lock));

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

    ULOCK(&(mb_ctx->mb_resc.lock));

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return length;
}

void evoc_mb_stay_set(EVOCMB_CTX_T *mb_ctx, UINT8_T stay)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mb_ctx);

    LOCK(&(mb_ctx->mb_resc.lock));

    mb_ctx->mb_resc.stay = stay;

    ULOCK(&(mb_ctx->mb_resc.lock));

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

void evoc_mb_stay_get(EVOCMB_CTX_T *mb_ctx, UINT8_T *stay)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mb_ctx);

    LOCK(&(mb_ctx->mb_resc.lock));

    *stay = mb_ctx->mb_resc.stay;

    ULOCK(&(mb_ctx->mb_resc.lock));

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
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