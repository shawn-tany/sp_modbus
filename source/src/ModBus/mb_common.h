/*
 * Author   : tanxiaoyang
 * Company  : EVOC
 * Function : encap/decap ModBus PDU(Protocol Data Unit) 
 */

#ifndef MB_DATA
#define MB_DATA

/* Base type define */
typedef unsigned char       UINT8_T;
typedef unsigned short      UINT16_T;
typedef unsigned int        UINT32_T;
typedef unsigned long long  UINT64_T;

/* point check */
#define PTR_CHECK_VOID(p)   \
    if (!p) {               \
        return ;            \
    }

#define PTR_CHECK_N1(p)     \
    if (!p) {               \
        return -1;          \
    }

#define PTR_CHECK_0(p)      \
    if (!p) {               \
        return 0;           \
    }

#define PTR_CHECK_NULL(p)   \
    if (!p) {               \
        return NULL;        \
    }

#ifdef MB_DEBUG
    #define MB_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else  
    #define MB_PRINT(format, ...) do {} while(0);
#endif

/* array element number */
#define ITEM(a) (sizeof(a) / sizeof(a[0]))

/* alignment X */
#define ALIGNED(value, align) ((value + align - 1) / align)

/* insert a byte to modbus data */
#define MBDATA_BYTE_SET(mb_data, byte)                              \
{                                                                   \
    mb_data->operate_data_len += 1;                                 \
    if ((mb_data->data_len + 1) <= mb_data->max_data_len)           \
    {                                                               \
        *(mb_data->data + mb_data->data_len) = byte;                \
        mb_data->data_len += 1;                                     \
    }                                                               \
}

/* insert a word to modbus data */
#define MBDATA_WORD_SET(mb_data, word)                              \
{                                                                   \
    mb_data->operate_data_len += 2;                                 \
    if ((mb_data->data_len + 2) <= mb_data->max_data_len)           \
    {                                                               \
        *((UINT16_T *)(mb_data->data + mb_data->data_len)) = word;  \
        mb_data->data_len += 2;                                     \
    }                                                               \
}

/* get a byte from modbus data */
#define MBDATA_BYTE_GET(mb_data, byte)                              \
{                                                                   \
    mb_data->operate_data_len += 1;                                 \
    if ((mb_data->offset + 1) <= mb_data->data_len)                 \
    {                                                               \
        byte = *(mb_data->data + mb_data->offset);                  \
        mb_data->offset += 1;                                       \
    }                                                               \
}

/* get a word from modbus data */
#define MBDATA_WORD_GET(mb_data, word)                              \
{                                                                   \
    mb_data->operate_data_len += 2;                                 \
    if ((mb_data->offset + 2) <= mb_data->data_len)                 \
    {                                                               \
        word = *((UINT16_T *)(mb_data->data + mb_data->offset));    \
        mb_data->offset += 2;                                       \
    }                                                               \
}

typedef enum
{
    MB_FUNC_01 = 0x01, 
    MB_FUNC_02 = 0x02, 
    MB_FUNC_03 = 0x03, 
    MB_FUNC_04 = 0x04, 
    MB_FUNC_05 = 0x05, 
    MB_FUNC_06 = 0x06, 
    MB_FUNC_0f = 0x0f, 
    MB_FUNC_10 = 0x10
} MB_CODE_T;

typedef enum
{
    MB_ERR_FUNC         = 0x01,
    MB_ERR_ADDR         = 0x02,
    MB_ERR_DATA         = 0x03,
    MB_ERR_SERVER       = 0x04,
    MB_ERR_WAIT         = 0x05,
    MB_ERR_BUSY         = 0x06,
    MB_ERR_UNREACHABLE  = 0x0A,
    MB_ERR_UNRESPONSIVE = 0x0B
} MB_ERR_T;

typedef enum 
{
    MB_RX = 0,
    MB_TX,
    MB_DIRECT_NUM,
} MB_DIRECT_T;

typedef enum
{
    MB_TYPE_TCP = 0,
    MB_TYPE_RTU
} MB_TYPE_T;

typedef union
{
    struct
    {
        UINT8_T high8;
        UINT8_T low8;
    } B2;
    
    UINT16_T W1;
} __attribute__((packed)) MB_WORD_T;

#define MAX_MBVALUE_SIZE 2000 /* byte */

typedef struct 
{
    MB_CODE_T code;
    MB_ERR_T  err;
    UINT16_T  reg;
    UINT16_T  n_reg;
    UINT8_T   n_byte;
    UINT8_T   value[MAX_MBVALUE_SIZE];
} __attribute__((packed)) MB_INFO_T;

typedef struct 
{
    /* data endian */
    UINT8_T is_big_endian;
    
    /* modbus info */
    MB_INFO_T mb_info;

    /* modbus cache */
    UINT16_T  max_data_len;     /* ModBus data cache size */
    UINT16_T  operate_data_len; /* ModBus data length you want to write/read, other than real data length */
    UINT16_T  data_len;         /* ModBus write offset & send/recv data length */
    UINT16_T  offset;           /* ModBus read offset */
    UINT8_T   data[0];
} __attribute__((packed)) MB_DATA_T;

/*
 * Function  : little endian to big endian for word
 * value     : value to be converted
 * return    : Converted value
 */
UINT16_T l2b_endian(UINT16_T value);

/*
 * Function  : big endian to little endian for word
 * value     : value to be converted
 * return    : Converted value
 */
UINT16_T b2l_endian(UINT16_T value);

/*
 * Function      : Create a cache for ModBus data transmission and reception
 * max_data_size : size of cache
 * return        : (MB_DATA_T *)=SUCCESS NULL=ERROR
 */
MB_DATA_T *mb_data_create(UINT32_T max_data_size);

/*
 * Function : destory the ModBus cache
 * mb_data  : the ModBus cache you want to destory
 * return   : void
 */
void mb_data_destory(MB_DATA_T *mb_data);

/*
 * Function : clear the ModBus cache
 * mb_data  : the ModBus cache you want to clear
 * return   : void
 */
void mb_data_clear(MB_DATA_T *mb_data);

/*
 * Function : encap the Modbus PDU
 * mb_data  : ModBus cache
 * return   : void
 */
void mb_data_encap(MB_DATA_T *mb_data);

/*
 * Function : decap the Modbus PDU
 * mb_data  : ModBus cache
 * return   : void
 */
void mb_data_decap(MB_DATA_T *mb_data);

/*
 * Function  : show ModBus data in cache
 * mb_data   : ModBus data cache
 * return    : void
 */
void mb_cache_show(MB_DATA_T *mb_data);

#endif
