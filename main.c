/*
 * Author   : tanxiaoyang
 * Company  : EVOC
 * Function : Debugging ModBus function
 */

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>

#include "evoc_mb.h"

#define EVOCMB_QUIT (88)

static UINT8_T running = 1;

EVOCMB_CTL_T default_ctl = {
    .mb_type = MB_TYPE_RTU,
    .max_data_size = 1400,
    .slaver_addr = 1,

    .tcp_ctrl = {
        .port = 502,
        .ip   = "192.168.1.12",
        .ethdev = "enp1s0"
    },

    .rtu_ctrl = {
        .baudrate = 9600,
        .databit  = 8,
        .stopbit  = 1,
        .flowctl  = 0,
        .parity   = 0,
        .serial   = "/dev/ttyUSB0"
    }
};

enum
{
    EVOCMB_OPT_TYPE = 1001,
    EVOCMB_OPT_MAX_DATA_SIZE,
    EVOCMB_OPT_IP,
    EVOCMB_OPT_PORT,
    EVOCMB_OPT_ETHDEV,
    EVOCMB_OPT_BAUDRATE,
    EVOCMB_OPT_DATABIT,
    EVOCMB_OPT_STOPBIT,
    EVOCMB_OPT_SERIAL,
    EVOCMB_OPT_FLOWCTL,
    EVOCMB_OPT_PARITY,
    EVOCMB_OPT_SLAVER,
    EVOCMB_OPT_HELP
};

static struct option long_options[] = {
    { "type",             1, 0, EVOCMB_OPT_TYPE             },
    { "max_data_size",    1, 0, EVOCMB_OPT_MAX_DATA_SIZE    },
    { "ip",               1, 0, EVOCMB_OPT_IP               },
    { "port",             1, 0, EVOCMB_OPT_PORT             },
    { "ethdev",           1, 0, EVOCMB_OPT_ETHDEV           },
    { "baudrate",         1, 0, EVOCMB_OPT_BAUDRATE         },
    { "databit",          1, 0, EVOCMB_OPT_DATABIT          },
    { "stopbit",          1, 0, EVOCMB_OPT_STOPBIT          },
    { "serial",           1, 0, EVOCMB_OPT_SERIAL           },
    { "flowctl",          1, 0, EVOCMB_OPT_FLOWCTL          },
    { "parity",           1, 0, EVOCMB_OPT_PARITY           },
    { "slaver",           1, 0, EVOCMB_OPT_SLAVER           },
    { "help",             0, 0, EVOCMB_OPT_HELP             }
};

/*
 * Function  : show ModBus demo option of command line
 * Parameter : void
 * return    : void
 */
static void help(void)
{
    printf( "\nOPTIONS :\n"
            "   --type,            Select ModBus protocol type\n"
            "   --max_data_size,   Limit ModBus transform data cache size\n"
            "   --ip,              ModBus TCP server ip\n"
            "   --port,            ModBus TCP server port\n"
            "   --ethdev,          ModBus TCP ethernet device for transform\n"
            "   --serial,          Select ModBus RTU serial\n"
            "   --buadrate,        Set ModBus RTU baudrate\n"
            "   --databit,         Set ModBus RTU data bit\n"
            "   --stopbit,         Set ModBus RTU stop bit\n"
            "   --flowctl,         Set ModBus RTU flow control\n"
            "   --parity,          Set ModBus RTU parity\n"
            "   --slaver,          Set ModBus RTU slaver address\n"
            "   --help,            Show EVOC ModBus demo options\n\n");
}

/*
 * Function  : parse ModBus demo option of command line
 * argc      : parameter number of command line
 * argv      : parameter list of command line
 * return    : 0=SUCCESS -1=ERROR
 */
static int arg_parse(int argc, char *argv[ ], EVOCMB_CTL_T *ctl)
{
    PTR_CHECK_N1(argv);
    PTR_CHECK_N1(ctl);

    int opt = 0;
    int idx = 0;

    while (-1 != (opt = getopt_long(argc, argv, "", long_options, &idx)))
    {
        switch (opt)
        {
            case EVOCMB_OPT_TYPE :
                if (!strcasecmp(optarg, "rtu"))
                {
                    ctl->mb_type = MB_TYPE_RTU;
                }
                else if (!strcasecmp(optarg, "tcp"))
                {
                    ctl->mb_type = MB_TYPE_TCP;
                }
                else
                {
                    printf("invalid modbus protocol type %s\n", optarg);
                    exit(2);
                }
                break;

            case EVOCMB_OPT_MAX_DATA_SIZE :
                ctl->max_data_size = strtol(optarg, NULL, 10);
                break;
        
            case EVOCMB_OPT_SLAVER :
                ctl->slaver_addr = strtol(optarg, NULL, 0);
                break;

            case EVOCMB_OPT_IP :
                snprintf(ctl->tcp_ctrl.ip, sizeof(ctl->tcp_ctrl.ip), "%s", optarg);
                break;
                
            case EVOCMB_OPT_PORT :
                ctl->tcp_ctrl.port = strtol(optarg, NULL, 10);
                break;
                
            case EVOCMB_OPT_ETHDEV :
                snprintf(ctl->tcp_ctrl.ethdev, sizeof(ctl->tcp_ctrl.ethdev), "%s", optarg);
                break;

            case EVOCMB_OPT_HELP :
                help();
                exit(0);

            case EVOCMB_OPT_SERIAL :
                snprintf(ctl->rtu_ctrl.serial, sizeof(ctl->rtu_ctrl.serial), "%s", optarg);
                break;

            case EVOCMB_OPT_BAUDRATE :
                ctl->rtu_ctrl.baudrate = strtol(optarg, NULL, 10);
                break;

            case EVOCMB_OPT_DATABIT :
                ctl->rtu_ctrl.databit = strtol(optarg, NULL, 0);
                break;

            case EVOCMB_OPT_STOPBIT :
                ctl->rtu_ctrl.stopbit = strtol(optarg, NULL, 0);
                break;

            case EVOCMB_OPT_FLOWCTL :
                ctl->rtu_ctrl.flowctl = strtol(optarg, NULL, 0);
                break;

            case EVOCMB_OPT_PARITY :
                ctl->rtu_ctrl.parity = strtol(optarg, NULL, 0);
                break;

            default :
                printf("invalid param %s\n", argv[idx]);
                help();
                exit(2);
        }
    }

    return 0;
}

/*
 * Function       : Select a ModBus function, and fill in relevant data
 * mb_master_info : the data information to be ModBus sent to the ModBus slaver from master
 * return         : (ModBus function code)=SUCCESS -1=ERROR
 */
static int command_select(MB_MASTER_T *mb_master_info)
{
    PTR_CHECK_N1(mb_master_info);

    int i = 0;
    UINT8_T  code = 0;
    char commad[32] = {0};

    memset(mb_master_info, 0, sizeof(MB_MASTER_T));

    printf( "\n"
            "   *********************************************************\n"
            "   *                          MENU                         *\n"
            "   *********************************************************\n"
            "   *  [   1]. Function code : 0x01 Read output IO coils    *\n"
            "   *  [   2]. FunCtion code : 0x02 Read output IO coils    *\n"
            "   *  [   3]. FunCtion code : 0x03 Read hold register      *\n"
            "   *  [   4]. FunCtion code : 0x04 Read input register     *\n"
            "   *  [   5]. FunCtion code : 0x05 Write a coil            *\n"
            "   *  [   6]. FunCtion code : 0x06 Write a register        *\n"
            "   *  [  15]. FunCtion code : 0x0F Write multiple coils    *\n"
            "   *  [  16]. FunCtion code : 0x10 Write multiple register *\n"
            "   *  [exit]. Exit                                         *\n"
            "   *********************************************************\n\n");
    
    printf("Please input code:\n");
    scanf("%s", commad);

    if (!strcasecmp(commad, "exit"))
    {
        code = EVOCMB_QUIT;
    }
    else
    {
        code = strtol(commad, NULL, 10);
    }

    switch (code)
    {
        case MB_FUNC_01 : 
        case MB_FUNC_02 : 
        case MB_FUNC_03 :
        case MB_FUNC_04 :
            printf("Please input hexadecimal register address\n");
            scanf("%hx", &mb_master_info->reg);

            printf("Please input register number\n");
            scanf("%hd", &mb_master_info->n_reg);
            break;
            
        case MB_FUNC_05 :
        case MB_FUNC_06 : 
            printf("Please input hexadecimal register address\n");
            scanf("%hx", &mb_master_info->reg);

            printf("Please input hexadecimal register value\n");
            scanf("%hx", (UINT16_T *)(&mb_master_info->value[0]));
            break;
            
        case MB_FUNC_0f : 
            printf("Please input hexadecimal register address\n");
            scanf("%hx", &mb_master_info->reg);

            printf("Please input coils number\n");
            scanf("%hd", &mb_master_info->n_reg);

            for (i = 0; i < mb_master_info->n_reg; ++i)
            {
                printf("Please input %dth coil value\n", i + 1);
                scanf("%hhd", (UINT8_T *)(&mb_master_info->value[i]));
            }
            break;
            
        case MB_FUNC_10 : 
            printf("Please input hexadecimal register address\n");
            scanf("%hx", &mb_master_info->reg);

            printf("Please input register number\n");
            scanf("%hd", &mb_master_info->n_reg);

            for (i = 0; i < mb_master_info->n_reg; ++i)
            {
                printf("Please input %dth hexadecimal register value\n", i + 1);
                scanf("%hx", (UINT16_T *)(&mb_master_info->value[i * 2]));
            }
            break;
            
        case EVOCMB_QUIT :
            return EVOCMB_QUIT;
            
        default :
            printf("Invalid code %d\n", (int)mb_master_info->code);
            return -1;
    }

    return (mb_master_info->code = code);
}

static void evocmb_data_print(MB_MASTER_T mb_master_info)
{
    /* Show response status */
    mb_status_show(mb_master_info);

    /* Show response data */
    mb_data_show(mb_master_info);
}

/*
 * Function       : work entrance of ModBus demo
 * mb_master_info : debugging all functions of ModBus demo
 * return         : (ModBus function code)=SUCCESS -1=ERROR
 */
static int work(MB_CTX_T *mb_ctx)
{
    PTR_CHECK_N1(mb_ctx);

    int ret = 0;

    while (running)
    {
        MB_MASTER_T mb_master_info;

        /* select a command */
        if (0 > (ret = command_select(&mb_master_info)))
        {
            printf("ERROR : Command set failed\n");
            continue;
        }
        
        if (EVOCMB_QUIT == ret)
        {
            break;
        }

        /* updata master info */
        if (0 > evoc_mbctx_master_updata(mb_ctx, mb_master_info))
        {
            printf("ERROR : ModBus context updata master info failed\n");
            continue;
        }

        /* send a modbsu request */
        if (0 > evoc_mb_send(mb_ctx))
        {
            printf("ERROR : ModBus send request failed\n");
            continue;
        }

        /* recv a modbus response */
        if (0 > evoc_mb_recv(mb_ctx))
        {
            printf("ERROR : ModBus recv response failed\n");
            continue;
        }

        /* take out modbus master info from slave respose */
        if (0 > evoc_mbctx_master_takeout(mb_ctx, &mb_master_info))
        {
            printf("ERROR : Modbus master info take out from context failed\n");
            continue;
        }

        /* Show response */
        evocmb_data_print(mb_master_info);
    }

    return 0;
}

int main(int argc, char *argv[ ])
{
    MB_CTX_T *mb_ctx = NULL;    
    EVOCMB_CTL_T ctl = default_ctl;

    arg_parse(argc, argv, &ctl);

    mb_ctx = evoc_mb_init(&ctl);
    if (!mb_ctx)
    {
        printf("Can not create evoc modbus context\n");
        return -1;
    }

    /* evoc modbus work */
    work(mb_ctx);

    evoc_mb_close(mb_ctx);
    
    printf("EVOC ModBus demo exit\n");

    return 0;
}
