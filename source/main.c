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

enum
{
    EVOCMB_QUIT = 0x100,
    EVOCMB_STAY,
    EVOCMB_UNSTAY,
};

static struct 
{
    char    *cmd;
    UINT16_T code;
} cmd_code_map[] = {
    { "exit",   EVOCMB_QUIT },
    { "stay",   EVOCMB_STAY },
    { "unstay", EVOCMB_UNSTAY }
};

static UINT8_T running = 1;

EVOCMB_CTL_T default_ctl = {
    .mb_type = MB_TYPE_RTU,

    .tcp_ctrl = {
        .port          = 502,
        .ip            = "192.168.1.12",
        .ethdev        = "enp1s0",
        .max_data_size = 1400,
        .unitid        = 1,
    },

    .rtu_ctrl = {
        .baudrate      = 9600,
        .databit       = 8,
        .stopbit       = 1,
        .flowctl       = 0,
        .parity        = 0,
        .serial        = "/dev/ttyUSB0",
        .max_data_size = 1400,
        .slaver_addr   = 1,
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
 * Function  : show ModBus demo option of commad line
 * Parameter : void
 * return    : void
 */
static void help(void)
{
    printf( "\nOPTIONS :\n"
            "   --type,            Select ModBus protocol type [tcp|rtu]\n"
            "   --max_data_size,   Limit ModBus transform data cache size [1400]\n"
            "   --ip,              ModBus TCP server ip [192.168.1.12]\n"
            "   --port,            ModBus TCP server port [502]\n"
            "   --ethdev,          ModBus TCP ethernet device for transform [eth0]\n"
            "   --serial,          Select ModBus RTU serial [/dev/ttyUSB0]\n"
            "   --buadrate,        Set ModBus RTU baudrate [9600]\n"
            "   --databit,         Set ModBus RTU data bit [8]\n"
            "   --stopbit,         Set ModBus RTU stop bit [1]\n"
            "   --flowctl,         Set ModBus RTU flow control [0]\n"
            "   --parity,          Set ModBus RTU parity [0]\n"
            "   --slaver,          Set ModBus RTU slaver address [1]\n"
            "   --help,            Show EVOC ModBus demo options\n\n");
}

/*
 * Function  : parse ModBus demo option of commad line
 * argc      : parameter number of commad line
 * argv      : parameter list of commad line
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
                ctl->tcp_ctrl.max_data_size = ctl->rtu_ctrl.max_data_size = strtol(optarg, NULL, 10);
                break;
        
            case EVOCMB_OPT_SLAVER :
                ctl->tcp_ctrl.unitid = ctl->rtu_ctrl.slaver_addr = strtol(optarg, NULL, 0);
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
 * Function     : Select a ModBus function, and fill in relevant data
 * mb_info      : the data information to be ModBus sent to the ModBus slaver from master
 * return       : (ModBus function code)=SUCCESS -1=ERROR
 */
static int commad_select(MB_INFO_T *mb_info)
{
    PTR_CHECK_N1(mb_info);

    int i = 0;
    UINT16_T  code = 0;
    char commad[32] = {0};

    memset(mb_info, 0, sizeof(MB_INFO_T));

    printf( "   ***********************************************************\n"
            "   *                          MENU                           *\n"
            "   ***********************************************************\n"
            "   *  [     1]. Function code : 0x01 Read output IO coils    *\n"
            "   *  [     2]. FunCtion code : 0x02 Read output IO coils    *\n"
            "   *  [     3]. FunCtion code : 0x03 Read hold register      *\n"
            "   *  [     4]. FunCtion code : 0x04 Read input register     *\n"
            "   *  [     5]. FunCtion code : 0x05 Write a coil            *\n"
            "   *  [     6]. FunCtion code : 0x06 Write a register        *\n"
            "   *  [    15]. FunCtion code : 0x0F Write multiple coils    *\n"
            "   *  [    16]. FunCtion code : 0x10 Write multiple register *\n"
            "   *  [  stay]. Stay connect                                 *\n"
            "   *  [unstay]. Unstay connect                               *\n"
            "   *  [  exit]. Exit                                         *\n"
            "   ***********************************************************\n\n");
    
    printf("Please input code:\n");
    do 
    {
        fgets(commad, sizeof(commad), stdin);

        commad[strlen(commad) - 1] = 0;
        
        if (strlen(commad))
        {
            break;
        }
    } while (1);
    
    for (i = 0; i < ITEM(cmd_code_map); ++i)
    {
        if (!strncasecmp(commad, cmd_code_map[i].cmd, strlen(cmd_code_map[i].cmd)))
        {
            code = cmd_code_map[i].code;
            break;
        }
    }

    if (!code)
    {
        code = strtol(commad, NULL, 0);
    }

    switch (code)
    {
        case MB_FUNC_01 : 
        case MB_FUNC_02 : 
        case MB_FUNC_03 :
        case MB_FUNC_04 :
            printf("Please input hexadecimal register address\n");
            fgets(commad, sizeof(commad), stdin);
            mb_info->reg = strtol(commad, NULL, 0);

            printf("Please input register number\n");
            fgets(commad, sizeof(commad), stdin);
            mb_info->n_reg = strtol(commad, NULL, 0);
            break;
            
        case MB_FUNC_05 :
        case MB_FUNC_06 : 
            printf("Please input hexadecimal register address\n");
            fgets(commad, sizeof(commad), stdin);
            mb_info->reg = strtol(commad, NULL, 0);

            printf("Please input hexadecimal register value\n");
            fgets(commad, sizeof(commad), stdin);
            *((UINT16_T *)(&mb_info->value[0])) = strtol(commad, NULL, 0);
            break;
            
        case MB_FUNC_0f : 
            printf("Please input hexadecimal register address\n");
            fgets(commad, sizeof(commad), stdin);
            mb_info->reg = strtol(commad, NULL, 0);

            printf("Please input coils number\n");
            fgets(commad, sizeof(commad), stdin);
            mb_info->n_reg = strtol(commad, NULL, 0);

            for (i = 0; i < mb_info->n_reg; ++i)
            {
                printf("Please input %dth coil value\n", i + 1);
                fgets(commad, sizeof(commad), stdin);
                mb_info->value[i] = strtol(commad, NULL, 0);
            }
            break;
            
        case MB_FUNC_10 : 
            printf("Please input hexadecimal register address\n");
            fgets(commad, sizeof(commad), stdin);
            mb_info->reg = strtol(commad, NULL, 0);

            printf("Please input register number\n");
            fgets(commad, sizeof(commad), stdin);
            mb_info->n_reg = strtol(commad, NULL, 0);

            for (i = 0; i < mb_info->n_reg; ++i)
            {
                printf("Please input %dth hexadecimal register value\n", i + 1);
                fgets(commad, sizeof(commad), stdin);
                *((UINT16_T *)(&mb_info->value[i * 2])) = strtol(commad, NULL, 0);
            }
            break;
            
        case EVOCMB_STAY :
        case EVOCMB_UNSTAY :
        case EVOCMB_QUIT :
            return code;
            
        default :
            printf("Invalid command(%s) code(%d)\n", commad, (int)mb_info->code);
            return -1;
    }

    return (mb_info->code = code);
}

static void work_mode_show(EVOCMB_CTX_T *mb_ctx)
{
    PTR_CHECK_VOID(mb_ctx);

    UINT8_T stay = 0;

    evoc_mb_stay_get(mb_ctx, &stay);

    printf("\n"
            "   ***********************************************************\n"
            "   * [ work mode : %-6s ]                                  *\n", stay ? "stay" : "unstay");
}

static void response_show(MB_INFO_T mb_info)
{
    /* Show response status */
    evoc_mb_status_show(mb_info);

    /* Show response data */
    evoc_mb_data_show(mb_info);
}

/*
 * Function : work entrance of ModBus demo
 * mb_info  : debugging all functions of ModBus demo
 * return   : (ModBus function code)=SUCCESS -1=ERROR
 */
static int work(EVOCMB_CTX_T *mb_ctx)
{
    PTR_CHECK_N1(mb_ctx);

    int       ret  = 0;
    MB_INFO_T mb_info;

    while (running)
    {
        work_mode_show(mb_ctx);

        /* select a commad */
        if (0 > (ret = commad_select(&mb_info)))
        {
            printf("ERROR : commad set failed\n");
            continue;
        }
        
        if (EVOCMB_QUIT == ret)
        {
            break;
        }

        if (EVOCMB_STAY == ret || EVOCMB_UNSTAY == ret)
        {
            evoc_mb_stay_set(mb_ctx, (EVOCMB_STAY == ret));
            continue;
        }

        /* send a modbsu request */
        if (0 > evoc_mb_send(mb_ctx, &mb_info))
        {
            printf("ERROR : ModBus send request failed\n");
            continue;
        }

        /* recv a modbus response */
        if (0 > evoc_mb_recv(mb_ctx, &mb_info))
        {
            printf("ERROR : ModBus recv response failed\n");
            continue;
        }

        /* Show response */
        response_show(mb_info);
    }

    return 0;
}

int main(int argc, char *argv[ ])
{
    EVOCMB_CTX_T *mb_ctx = NULL;    
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
