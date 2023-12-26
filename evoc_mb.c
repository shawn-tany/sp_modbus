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

static void *stay_connected_routine(void *arg)
{
    PTR_CHECK_NULL(arg);

    pthread_detach(pthread_self());

    MB_CTX_T *mb_ctx = (MB_CTX_T *)arg;

#ifdef MB_STAY
    while (1)
#else
    while (0)
#endif
    {
        sleep(1);

        MB_MASTER_T mb_master_info;

        memset(&mb_master_info, 0, sizeof(MB_MASTER_T));
        mb_master_info.code    = MB_FUNC_01;
        mb_master_info.reg     = 0x0000;
        mb_master_info.n_reg   = 16;

        /* updata master info */
        if (0 > evoc_mbctx_master_updata(mb_ctx, mb_master_info))
        {
            MB_PRINT("THREAD ERROR : ModBus context updata master info failed\n");
            continue;
        }

        /* send a modbsu request */
        if (0 > evoc_mb_send(mb_ctx))
        {
            MB_PRINT("THREAD ERROR : ModBus send request failed\n");
            continue;
        }

        /* recv a modbus response */
        if (0 > evoc_mb_recv(mb_ctx))
        {
            MB_PRINT("THREAD ERROR : ModBus recv response failed\n");
            continue;
        }

        /* take out modbus master info from slave respose */
        if (0 > evoc_mbctx_master_takeout(mb_ctx, &mb_master_info))
        {
            MB_PRINT("THREAD ERROR : Modbus master info take out from context failed\n");
            continue;
        }
    }

    return NULL;
}

/*
 * Function  : Create a ModBus Context
 * mb_ctl    : some information for create modBus Context
 * return    : (MB_CTX_T *)=SUCCESS NULL=ERRROR
 */
MB_CTX_T *evoc_mb_init(EVOCMB_CTL_T *mb_ctl)
{
    MB_CTX_T *mb_ctx = NULL;

    /* create evoc modbus context */
    mb_ctx = (MB_CTX_T *)malloc(sizeof(MB_CTX_T));
    if (!mb_ctx)
    {
        return NULL;
    }
    memset(mb_ctx, 0, sizeof(MB_CTX_T));

    /* create evoc modbus data cache */
    mb_ctx->mb_data = mb_data_create(mb_ctl->max_data_size);
    if (!(mb_ctx->mb_data))
    {
        free(mb_ctx);
        return NULL;
    }
    mb_ctx->mb_data->slave_addr = mb_ctl->slaver_addr;

    /* create a modbus descriptor */
    if (MB_TYPE_TCP == mb_ctl->mb_type)
    {
        mb_ctx->mb_type = MB_TYPE_TCP;
        mb_ctx->mb_desc = mb_tcp_init(&mb_ctl->tcp_ctrl);
    }
    else if (MB_TYPE_RTU == mb_ctl->mb_type)
    {
        mb_ctx->mb_type = MB_TYPE_RTU;
        mb_ctx->mb_desc = mb_rtu_init(&mb_ctl->rtu_ctrl);
    }

    /* check descriptor */
    if (!mb_ctx->mb_desc)
    {
        free(mb_ctx->mb_data);
        free(mb_ctx);
        return NULL;
    }

    /* ModBus stay connected */
    if (0 > pthread_create(&(mb_ctx->mb_resc.pid), NULL, stay_connected_routine, mb_ctx))
    {
        perror("thread create error");
        free(mb_ctx->mb_data);
        free(mb_ctx);
        return NULL;
    }

    /* Create mutex lock */
    pthread_mutex_init(&(mb_ctx->mb_resc.lock), NULL);

    return mb_ctx;
}

/*
 * Function : close a ModBus Context
 * mb_ctx   : the ModBus context you want to close
 * return   : void
 */
void evoc_mb_close(MB_CTX_T *mb_ctx)
{
    LOCK(&(mb_ctx->mb_resc.lock));

    PTR_CHECK_VOID(mb_ctx);

    /* close modbus descriptor */
    if (MB_TYPE_TCP == mb_ctx->mb_type)
    {
        mb_tcp_close(mb_ctx->mb_desc);
    }
    else if (MB_TYPE_RTU == mb_ctx->mb_type)
    {
        mb_rtu_close(mb_ctx->mb_desc);
    }

    mb_data_destory(mb_ctx->mb_data);

    free(mb_ctx);

    ULOCK(&(mb_ctx->mb_resc.lock));
}

/*
 * Function : recv ModBus data from slaver
 * mb_ctx   : ModBus context
 * return   : 0=CLOSE length=SUCCESS -1=ERROR
 */
int evoc_mb_recv(MB_CTX_T *mb_ctx)
{
    LOCK(&(mb_ctx->mb_resc.lock));

    PTR_CHECK_N1(mb_ctx);

    int length = 0;

    /* recv modbus data */
    if (MB_TYPE_TCP == mb_ctx->mb_type)
    {
        length = mb_tcp_recv(mb_ctx->mb_desc, mb_ctx->mb_data);
    }
    else if (MB_TYPE_RTU == mb_ctx->mb_type)
    {
        length = mb_rtu_recv(mb_ctx->mb_desc, mb_ctx->mb_data);
    }

    ULOCK(&(mb_ctx->mb_resc.lock));

    return length;
}

/*
 * Function : send ModBus data to slaver
 * mb_ctx   : ModBus context
 * return   : 0=CLOSE length=SUCCESS -1=ERROR
 */
int evoc_mb_send(MB_CTX_T *mb_ctx)
{
    LOCK(&(mb_ctx->mb_resc.lock));

    PTR_CHECK_N1(mb_ctx);

    int length = 0;

    /* send modbus data */
    if (MB_TYPE_TCP == mb_ctx->mb_type)
    {
        length = mb_tcp_send(mb_ctx->mb_desc, mb_ctx->mb_data);
    }
    else if (MB_TYPE_RTU == mb_ctx->mb_type)
    {
        length = mb_rtu_send(mb_ctx->mb_desc, mb_ctx->mb_data);
    }

    ULOCK(&(mb_ctx->mb_resc.lock));

    return length;
}

/*
 * Function : updata ModBus master information to ModBus context
 * mb_ctx   : ModBus context
 * return   : 0=SUCCESS -1=ERROR
 */
int evoc_mbctx_master_updata(MB_CTX_T *mb_ctx, MB_MASTER_T mb_master_info)
{
    LOCK(&(mb_ctx->mb_resc.lock));

    PTR_CHECK_N1(mb_ctx);

    switch (mb_master_info.code)
    {
        case MB_FUNC_01 : 
        case MB_FUNC_02 : 
        case MB_FUNC_03 :
        case MB_FUNC_04 :
            if (!mb_master_info.n_reg)
            {
                return -1;
            }
            break;
            
        case MB_FUNC_05 :
        case MB_FUNC_06 : 
            break;
            
        case MB_FUNC_0f : 
        case MB_FUNC_10 : 
            if (!mb_master_info.n_reg)
            {
                return -1;
            }
            break;
            
        default :
            return -1; 
    }

    /* updata master info */
    mb_ctx->mb_data->master_info = mb_master_info;
    
    ULOCK(&(mb_ctx->mb_resc.lock));

    return 0;
}

/*
 * Function : take out ModBus master information from ModBus context
 * mb_ctx   : ModBus context
 * return   : 0=SUCCESS -1=ERROR
 */
int evoc_mbctx_master_takeout(MB_CTX_T *mb_ctx, MB_MASTER_T *mb_master_info)
{
    LOCK(&(mb_ctx->mb_resc.lock));

    PTR_CHECK_N1(mb_ctx);
    PTR_CHECK_N1(mb_master_info);

    /* take out master info */
    *mb_master_info = mb_ctx->mb_data->master_info;

    if (mb_master_info->err)
    {
        return -1;
    }
    
    ULOCK(&(mb_ctx->mb_resc.lock));

    return 0;
}

/*
 * Function         : show response status from ModBus slaver
 * mb_master_info   : ModBus master info
 * return           : void
 */
void mb_status_show(MB_MASTER_T mb_master_info)
{
    if (!(mb_master_info.err))
    {
        printf("MB SUCCESS\n");
        return ;
    }

    UINT8_T code = mb_master_info.code & (~0x80);
    int i = 0;

    printf("MB ERROR CODE(0x%02x) : ", mb_master_info.err);

    switch (mb_master_info.err)
    {
        case MB_ERR_FUNC :
            printf("Invalid function code(0x%02x)\n", code);
            break;

        case MB_ERR_ADDR :
            printf("Invalid register address(0x%02x)\n", mb_master_info.reg);
            break;

        case MB_ERR_DATA :
            printf("Invalid data\n");
            for (i = 0; i < mb_master_info.n_byte; ++i)
            {
                printf("Data[%d] : 0x%02x\n", i, mb_master_info.value[i]);
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
}

/*
 * Function         : show ModBus data after decap
 * mb_master_info   : ModBus master info
 * return           : void
 */
void mb_data_show(MB_MASTER_T mb_master_info)
{
    int i = 0;

    if (mb_master_info.err)
    {
        return ;
    }

    switch (mb_master_info.code)
    {
        case MB_FUNC_01 : 
        case MB_FUNC_02 : 
        case MB_FUNC_03 : 
        case MB_FUNC_04 : 
            printf("mb_master_info.code = %02x\n", mb_master_info.code);
            printf("mb_master_info.n_byte = %d\n", mb_master_info.n_byte);
            /* value */
            for (i = 0; i < mb_master_info.n_byte; i++)
            {
                printf("mb_master_info.value[%d] = %02x\n", i + 1, mb_master_info.value[i]);
            }
            break;
            
        case MB_FUNC_05 : 
        case MB_FUNC_06 : 
            break;
            
        case MB_FUNC_0f : 
        case MB_FUNC_10 : 
            break;
            
        default :
            break;
    }
}
