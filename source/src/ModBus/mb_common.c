/*
 * Author   : tanxiaoyang
 * Company  : EVOC
 * Function : encap/decap ModBus PDU(Protocol Data Unit) 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mb_common.h"

/*
 * Function  : endian check
 * parameter : void
 * return    : 0=(little endian) -1=(big endian)
 */
static UINT8_T is_big_endian(void)
{
    union
    {
        UINT16_T word;
        UINT8_T  byte;
    } endian_test;
    
    endian_test.word = 0x00ff;

    return (endian_test.byte == 0xff);
}

/*
 * Function  : little endian to big endian for word
 * value     : value to be converted
 * return    : Converted value
 */
UINT16_T l2b_endian(UINT16_T value)
{
    MB_WORD_T word1 = {
        .W1 = value
    };
    
    MB_WORD_T word2 = {
        .B2.high8 = word1.B2.low8,
        .B2.low8  = word1.B2.high8
    };
    
    return word2.W1;
}

/*
 * Function  : big endian to little endian for word
 * value     : value to be converted
 * return    : Converted value
 */
UINT16_T b2l_endian(UINT16_T value)
{
    MB_WORD_T word1 = {
        .W1 = value
    };
    
    MB_WORD_T word2 = {
        .B2.high8 = word1.B2.low8,
        .B2.low8  = word1.B2.high8
    };
    
    return word2.W1;
}

/*
 * Function      : Create a cache for ModBus data transmission and reception
 * max_data_size : size of cache
 * return        : (MB_DATA_T *)=SUCCESS NULL=ERROR
 */
MB_DATA_T *mb_data_create(UINT32_T max_data_size)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    MB_DATA_T *mb_data = NULL;

    /* create evoc modbus data cache */
    mb_data = (MB_DATA_T *)malloc(sizeof(MB_DATA_T) + max_data_size);
    if (!mb_data)
    {
        return NULL;
    }
    memset(mb_data, 0, (sizeof(MB_DATA_T) + max_data_size));

    mb_data->max_data_len  = max_data_size;
    mb_data->is_big_endian = is_big_endian();

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    return mb_data;
}

/*
 * Function : destory the ModBus cache
 * mb_data  : the ModBus cache you want to destory
 * return   : void
 */
void mb_data_destory(MB_DATA_T *mb_data)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mb_data);

    free(mb_data);

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

/*
 * Function : clear the ModBus cache
 * mb_data  : the ModBus cache you want to clear
 * return   : void
 */
void mb_data_clear(MB_DATA_T *mb_data)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mb_data);
    
    memset(mb_data->data, 0, mb_data->max_data_len);
    mb_data->data_len = 0;
    mb_data->offset   = 0;

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

/*
 * Function : encap the Modbus PDU
 * mb_data  : ModBus cache
 * return   : void
 */
void mb_data_encap(MB_DATA_T *mb_data)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mb_data);

    int i = 0;
    int j = 0;
    UINT8_T  byte = 0;
    UINT16_T word = 0;
    MB_INFO_T mb_info = mb_data->mb_info;

    /* common */
    {
        /* function code */
        MBDATA_BYTE_SET(mb_data, mb_info.code);

        /* register address */
        if (mb_data->is_big_endian)
        {
            mb_info.reg = l2b_endian(mb_info.reg);
        }
        MBDATA_WORD_SET(mb_data, mb_info.reg);
    }

    switch (mb_info.code)
    {
        case MB_FUNC_01 : 
        case MB_FUNC_02 : 
        case MB_FUNC_03 :
        case MB_FUNC_04 :
            /* register number */
            if (mb_data->is_big_endian)
            {
                mb_info.n_reg = l2b_endian(mb_info.n_reg);
            }
            MBDATA_WORD_SET(mb_data, mb_info.n_reg);
            break;
            
        case MB_FUNC_05 :
        case MB_FUNC_06 : 
            /* register value */
            word = *(UINT16_T *)(&mb_info.value[0]);
            if (!mb_data->is_big_endian)
            {
                word = l2b_endian(word);
            }
            MBDATA_WORD_SET(mb_data, word);
            break;
            
        case MB_FUNC_0f : 
            /* register number */
            if (mb_data->is_big_endian)
            {
                mb_info.n_reg = l2b_endian(mb_info.n_reg);
            }
            MBDATA_WORD_SET(mb_data, mb_info.n_reg);
            
            /* value byte number */
            mb_info.n_byte = ALIGNED(mb_data->mb_info.n_reg, 8);
            MBDATA_BYTE_SET(mb_data, mb_info.n_byte);
            
            /* value */
            for (i  = 0; i < mb_info.n_byte && i < ITEM(mb_info.value); ++i)
            {
                byte = 0;
                for (j = 0; j < 8; j++)
                {
                    byte |= (!!mb_info.value[(i * 8) + j]) << j;
                }
                MBDATA_BYTE_SET(mb_data, byte); 
            }
            break;
        
        case MB_FUNC_10 : 
            /* register number */
            if (mb_data->is_big_endian)
            {
                mb_info.n_reg = l2b_endian(mb_info.n_reg);
            }
            MBDATA_WORD_SET(mb_data, mb_info.n_reg);
            
            /* value byte number */
            mb_info.n_byte = mb_data->mb_info.n_reg * 2;
            MBDATA_BYTE_SET(mb_data, mb_info.n_byte);
            
            /* value */
            for (i  = 0; i < mb_info.n_byte && i < ITEM(mb_info.value); ++i)
            {
                MBDATA_BYTE_SET(mb_data, mb_info.value[i]); 
            }   
            break;
            
        default :
            break;
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

/*
 * Function : decap the Modbus PDU
 * mb_data  : ModBus cache
 * return   : void
 */
void mb_data_decap(MB_DATA_T *mb_data)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mb_data);

    int i = 0;
    MB_INFO_T *mb_info = &(mb_data->mb_info);

    /* common */
    {
        /* function code */
        MBDATA_BYTE_GET(mb_data, mb_info->code);
    }

    if (0x80 < mb_info->code)
    {
        /* error code */
        MBDATA_BYTE_GET(mb_data, mb_info->err);
        return ;
    }

    switch (mb_info->code)
    {
        case MB_FUNC_01 : 
        case MB_FUNC_02 : 
        case MB_FUNC_03 : 
        case MB_FUNC_04 :             
            /* value byte number */
            MBDATA_BYTE_GET(mb_data, mb_info->n_byte);
            
            /* value */
            for (i = 0; i < mb_info->n_byte; i++)
            {
                MBDATA_BYTE_GET(mb_data, mb_info->value[i]);
            }
            break;
            
        case MB_FUNC_05 : 
        case MB_FUNC_06 : 
            /* register address */
            MBDATA_WORD_GET(mb_data, mb_info->reg);
            if (mb_data->is_big_endian)
            {
                mb_info->reg = b2l_endian(mb_info->reg);
            }
        
            /* register value */
            MBDATA_BYTE_GET(mb_data, mb_info->value[0]);
            MBDATA_BYTE_GET(mb_data, mb_info->value[1]);
            break;
            
        case MB_FUNC_0f : 
        case MB_FUNC_10 : 
            /* register address */
            MBDATA_WORD_GET(mb_data, mb_info->reg);
            if (mb_data->is_big_endian)
            {
                mb_info->reg = b2l_endian(mb_info->reg);
            }
        
            /* register number */
            MBDATA_WORD_GET(mb_data, mb_info->n_reg);
            if (mb_data->is_big_endian)
            {
                mb_info->n_reg = b2l_endian(mb_info->n_reg);
            }
            break;
            
        default :
            break;
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}

/*
 * Function  : show ModBus data in cache
 * mb_data   : ModBus data cache
 * return    : void
 */
void mb_cache_show(MB_DATA_T *mb_data)
{
    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);

    PTR_CHECK_VOID(mb_data);

    int i = 0;

    for (i = 0; i < mb_data->data_len && i < mb_data->max_data_len; ++i)
    {
        printf("MB cache data[%d] : 0x%02x\n", i, mb_data->data[i]);
    }

    MB_PRINT("%s : %d\n", __FUNCTION__, __LINE__);
}