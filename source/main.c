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
#include <unistd.h>
#include <pthread.h>

#include "evoc_mb.h"
#include "mask_rule.h"

#define LOCK(lock)  pthread_mutex_lock(lock)
#define ULOCK(lock) pthread_mutex_unlock(lock)

#define MAX_MASK_RULE_NUM 128

enum
{
    EVOCMB_QUIT = 0x100,
    EVOCMB_STAY,
    EVOCMB_RELY,
    EVOCMB_RULE,
    EVOCMB_IO
};

static EVOCMB_CTX_T *mb_ctx  = NULL; 
static MASK_RULE_T  *ruleset = NULL;

static EVOCMB_CTL_T default_ctl = {
    .mb_type = MB_TYPE_TCP,

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

static struct 
{
    char    *cmd;
    UINT16_T code;
} cmd_code_map[] = {
    { "exit", EVOCMB_QUIT },
    { "stay", EVOCMB_STAY },
    { "rely", EVOCMB_RELY },
    { "rule", EVOCMB_RULE },
    { "io",   EVOCMB_IO   }
};

enum 
{
    STAY_THREAD = 0,
    RELY_THREAD = 1,
    THREAD_NUM  = 2
};

static struct 
{
    pthread_mutex_t lock;
    pthread_t       pid[THREAD_NUM];
    UINT8_T         stay;
    UINT8_T         rely;
    UINT8_T         running;
} resource = {
    .running = 1
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

static void work_mode_show(void)
{
    printf("\n"
            "   *********************************************************\n"
            "   * [ stay: %-3s ]                                         *\n"
            "   * [ rely: %-3s ]                                         *\n",
            resource.stay ? "on" : "off", 
            resource.rely ? "on" : "off");
}

static int command_funccode_handle(UINT8_T code, MB_INFO_T *mb_info)
{
    PTR_CHECK_N1(mb_info);

    int i   = 0;
    char command[32] = {0};

    switch (code)
    {
        case MB_FUNC_01 : 
        case MB_FUNC_02 : 
        case MB_FUNC_03 :
        case MB_FUNC_04 :
            printf("Please input hexadecimal register address\n");
            fgets(command, sizeof(command), stdin);
            mb_info->reg = strtol(command, NULL, 0);

            printf("Please input register number\n");
            fgets(command, sizeof(command), stdin);
            mb_info->n_reg = strtol(command, NULL, 0);
            break;
            
        case MB_FUNC_05 :
        case MB_FUNC_06 : 
            printf("Please input hexadecimal register address\n");
            fgets(command, sizeof(command), stdin);
            mb_info->reg = strtol(command, NULL, 0);

            printf("Please input hexadecimal register value\n");
            fgets(command, sizeof(command), stdin);
            *((UINT16_T *)(&mb_info->value[0])) = strtol(command, NULL, 0);
            break;
            
        case MB_FUNC_0f : 
            printf("Please input hexadecimal register address\n");
            fgets(command, sizeof(command), stdin);
            mb_info->reg = strtol(command, NULL, 0);

            printf("Please input coils number\n");
            fgets(command, sizeof(command), stdin);
            mb_info->n_reg = strtol(command, NULL, 0);

            for (i = 0; i < mb_info->n_reg; ++i)
            {
                printf("Please input %dth coil value\n", i + 1);
                fgets(command, sizeof(command), stdin);
                mb_info->value[i] = strtol(command, NULL, 0);
            }
            break;
            
        case MB_FUNC_10 : 
            printf("Please input hexadecimal register address\n");
            fgets(command, sizeof(command), stdin);
            mb_info->reg = strtol(command, NULL, 0);

            printf("Please input register number\n");
            fgets(command, sizeof(command), stdin);
            mb_info->n_reg = strtol(command, NULL, 0);

            for (i = 0; i < mb_info->n_reg; ++i)
            {
                printf("Please input %dth hexadecimal register value\n", i + 1);
                fgets(command, sizeof(command), stdin);
                *((UINT16_T *)(&mb_info->value[i * 2])) = strtol(command, NULL, 0);
            }
            break;
    }

    return (mb_info->code = code);
}

enum 
{
    MBRULE_ADD = 0,
    MBRULE_DEL = 1,
    MBRULE_GET = 2
};

static int command_io_handle(void)
{
    char   *ptr = NULL;
    char    command[32] = {0};
    char    argv[128][128];
    int     argc = 0;
    int     idx  = 0;
    int     ioidx = 0;
    IO_DIRECTION_T  direction = IO_OUTPUT;
    IO_STATUS_T     status    = IO_ON;

    while (1)
    {
        printf("Please input operation for io\n"
           "eg : get in  [id]\n"
           "     get out [id]\n"
           "     set [id] on\n"
           "     set [id] off\n"
           "     exit\n");
        do 
        {
            fgets(command, sizeof(command), stdin);

            command[strlen(command) - 1] = 0;
            
            if (strlen(command))
            {
                break;
            }
        } while (1);

        if (!strcmp(command, "exit"))
        {
            break;
        }

        ptr = strtok(command, " ");

        argc = 0;
        while (ptr)
        {
            snprintf(argv[argc++], sizeof(argv[argc++]), "%s", ptr);
            ptr = strtok(NULL, " ");
        }

        idx = 0;
        if (!strcmp(argv[idx], "get"))
        {
            idx++;

            direction = !strcmp(argv[idx], "in") ? IO_INPUT : IO_OUTPUT;
            idx++;

            ioidx = strtol(argv[idx], NULL, 0);
            
            evoc_mbio_get(mb_ctx, direction, ioidx, &status);

            printf("%s IO(%d) status(%s)\n", (direction == IO_INPUT) ? "in" : "out",
                ioidx, (status == IO_ON) ? "on" : "off");
        }
        else if (!strcmp(argv[idx], "set"))
        {
            idx++;

            ioidx = strtol(argv[idx], NULL, 0);
            idx++;

            status = !strcmp(argv[idx], "on") ? IO_ON : IO_OFF;
            
            evoc_mbio_set(mb_ctx, ioidx, status);
        }
    }

    return 0;
}

static int command_rule_handle(void)
{
    char *ptr = NULL;
    char  command[128] = {0};
    char  argv[128][128];
    int   argc = 0;
    int   id   = 0;
    int   i    = 0;
    int   idx  = 0;

    MASK_RULE_NODE_T node;
    const MASK_RULE_NODE_T *getnode = NULL;

    while (1)
    {
        memset(&node, 0, sizeof(node));

        printf("Please input operation for rule \n"
            "eg : get all\n"
            "     get [id]\n"
            "     add imask.up 0xff imask.dwon 0xff omask.up 0xff omask.down 0xff prio 7\n"
            "     add imask.up 0xff omask.up 0xff prio 7\n"
            "     add imask.up 0xff omask.down 0xff\n"
            "     del all\n"
            "     del [id]\n"
            "     exit\n");

        do 
        {
            fgets(command, sizeof(command), stdin);

            command[strlen(command) - 1] = 0;
            
            if (strlen(command))
            {
                break;
            }
        } while (1);

        if (!strcmp(command, "exit"))
        {
            break;
        }

        ptr = strtok(command, " ");

        while (ptr)
        {
            snprintf(argv[argc++], sizeof(argv[argc++]), "%s", ptr);
            ptr = strtok(NULL, " ");
        }

        if (!strcmp(argv[idx], "del"))
        {
            idx++;
            if (!strcasecmp(argv[idx], "all"))
            {
                /* delete all */
                for (i = 0; i < MAX_MASK_RULE_NUM; ++i)
                {
                    mask_rule_del(ruleset, i + 1);
                }
            }
            else
            {
                id = strtol(argv[idx], NULL, 0);

                /* delete */
                mask_rule_del(ruleset, id);
            }
        }
        else if (!strcmp(argv[idx], "get"))
        {
            idx++;
            if (!strcasecmp(argv[idx], "all"))
            {
                /* get all */
                for (i = 0; i < MAX_MASK_RULE_NUM; ++i)
                {
                    getnode = mask_rule_get(ruleset, i + 1);
                    if (getnode)
                    {
                        mask_rule_display(&getnode->content);
                    }
                }
            }
            else
            {
                id = strtol(argv[idx], NULL, 0);
                /* get */
                getnode = mask_rule_get(ruleset, id);
                if (getnode)
                {
                    mask_rule_display(&getnode->content);
                }
                else
                {
                    printf("ERROR : No such rule\n");
                    return 0;
                }
            }
        }
        else if (!strcmp(argv[idx], "add"))
        {
            idx++;
            if (!strcmp(argv[idx], "imask.up"))
            {
                idx++;
                node.content.imask.up = strtol(argv[idx++], NULL, 0);
            }

            if (!strcmp(argv[idx], "imask.down"))
            {
                idx++;
                node.content.imask.down = strtol(argv[idx++], NULL, 0);
            }
            
            if (!strcmp(argv[idx], "omask.up"))
            {
                idx++;
                node.content.omask.up = strtol(argv[idx++], NULL, 0);
            }

            if (!strcmp(argv[idx], "omask.down"))
            {
                idx++;
                node.content.omask.down = strtol(argv[idx++], NULL, 0);
            }

            if (!strcmp(argv[idx], "prio"))
            {
                idx++;
                node.content.priority = strtol(argv[idx++], NULL, 0);
            }

            /* check */
            if (!node.content.imask.up && !node.content.imask.down)
            {
                printf("ERROR : Invalid rule imask up(0x%llx) down(0x%llx)!\n", 
                    node.content.imask.up, node.content.imask.down);
                return -1;
            }

            if (node.content.imask.up & node.content.imask.down)
            {
                printf("ERROR : Invalid rule imask up(0x%llx) down(0x%llx)!\n", 
                    node.content.imask.up, node.content.imask.down);
                return -1;
            }

            if (!node.content.omask.up && !node.content.omask.down)
            {
                printf("ERROR : Invalid rule omask up(0x%llx) down(0x%llx)!\n", 
                    node.content.omask.up, node.content.omask.down);
                return -1;
            }

            if (node.content.omask.up & node.content.omask.down)
            {
                printf("ERROR : Invalid rule omask up(0x%llx) down(0x%llx)!\n", 
                    node.content.omask.up, node.content.omask.down);
                return -1;
            }

            if (MASK_RULE_PRIORITY_NUM <= node.content.priority)
            {
                printf("ERROR : Invalid rule priority %d!\n", node.content.priority);
                return -1;
            }

            /* add rule */
            if (0 > mask_rule_add(ruleset, node))
            {
                printf("ERROR : failed to add rule!\n");
                return -1;
            }
        }
    }

    return 0;
}

/*
 * Function     : Select a ModBus function, and fill in relevant data
 * mb_info      : the data information to be ModBus sent to the ModBus slaver from master
 * return       : (ModBus function code)=SUCCESS -1=ERROR
 */
static int command_select(MB_INFO_T *mb_info)
{
    PTR_CHECK_N1(mb_info);

    int i = 0;
    UINT16_T  code = 0;
    char command[32] = {0};

    memset(mb_info, 0, sizeof(MB_INFO_T));

    work_mode_show();

    printf( "   *********************************************************\n"
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
            "   *  [stay]. Stay connect state                           *\n"
            "   *  [rely]. IO rely control state                        *\n"
            "   *  [rule]. IO rely rule for IO control                  *\n"
            "   *  [  io]. IO control                                   *\n"
            "   *  [exit]. Exit                                         *\n"
            "   *********************************************************\n\n");
    
    printf("Please input code:\n");
    do 
    {
        fgets(command, sizeof(command), stdin);

        command[strlen(command) - 1] = 0;
        
        if (strlen(command))
        {
            break;
        }
    } while (1);
    
    /* format command */
    for (i = 0; i < ITEM(cmd_code_map); ++i)
    {
        if (!strcasecmp(command, cmd_code_map[i].cmd))
        {
            code = cmd_code_map[i].code;
            break;
        }
    }

    /* invalid command */
    if (!code)
    {
        code = strtol(command, NULL, 0);
    }

    /* ModBus function command */
    if (MB_FUNC_01 <= code && MB_FUNC_10 >= code)
    {
        return command_funccode_handle(code, mb_info);
    }

    switch (code)
    {
        case EVOCMB_STAY :
            printf("Please input stay status [on|off]\n");
            fgets(command, sizeof(command), stdin);
            resource.stay = strncasecmp(command, "on", 2) ? 0 : 1;
            break;

        case EVOCMB_RELY :
            printf("Please input rely status [on|off]\n");
            fgets(command, sizeof(command), stdin);
            resource.rely = strncasecmp(command, "on", 2) ? 0 : 1;
            break;

        case EVOCMB_RULE :
            return command_rule_handle();

        case EVOCMB_IO :
            return command_io_handle();

        case EVOCMB_QUIT :
            resource.running = 0;
            break;
            
        default :
            printf("Invalid command(%s) code(%d)\n", command, (int)mb_info->code);
            return -1;
    }

    return 0;
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

    while (resource.running)
    {
        /* select a command */
        if (0 >= (ret = command_select(&mb_info)))
        {
            if (ret)
            {
                printf("ERROR : command set failed\n");
            }
            continue;
        }

        LOCK(&resource.lock);

        do 
        {
            /* send a modbsu request */
            if (0 > evoc_mb_send(mb_ctx, &mb_info))
            {
                printf("ERROR : ModBus send request failed\n");
                break;
            }

            /* recv a modbus response */
            if (0 > evoc_mb_recv(mb_ctx, &mb_info))
            {
                printf("ERROR : ModBus recv response failed\n");
                break;
            }

            /* Show response status */
            evoc_mb_status_show(mb_info);

            /* Show response data */
            evoc_mb_data_show(mb_info);
        } while (0);

        ULOCK(&resource.lock);
    }

    return 0;
}

static void *stay_connected_routine(void *arg)
{
    MB_INFO_T mb_info = {
        .code  = MB_FUNC_01,
        .reg   = 0x0000,
        .n_reg = 16
    };

    while (resource.running)
    {
        sleep(1);

        LOCK(&resource.lock);

        do 
        {
            if (!(resource.stay) || (resource.rely))
            {
                break;
            }

            /* send a modbsu request */
            if (0 > evoc_mb_send(mb_ctx, &mb_info))
            {
                MB_PRINT("THREAD ERROR : ModBus send request failed\n");
                break;
            }

            /* recv a modbus response */
            if (0 > evoc_mb_recv(mb_ctx, &mb_info))
            {
                MB_PRINT("THREAD ERROR : ModBus recv response failed\n");
                break;
            }
        } while (0);

        ULOCK(&resource.lock);
    }

    return NULL;
}

static int io_rely_handle(MASK_RULE_CONTENT_T *content, void *)
{
    int i = 0;

    MB_INFO_T get_mb_info = {
        .code  = MB_FUNC_01,
        .reg   = 16,
        .n_reg = 16
    };

    MB_INFO_T set_mb_info = {
        .code  = MB_FUNC_0f,
        .reg   = 16,
        .n_reg = 16
    };

    mask_rule_display(content);

    /* get modbsu output coils request */
    if (0 > evoc_mb_send(mb_ctx, &get_mb_info))
    {
        MB_PRINT("IO RELY ERROR : ModBus send request failed\n");
        return -1;
    }

    /* get modbsu output coils response */
    if (0 > evoc_mb_recv(mb_ctx, &get_mb_info))
    {
        MB_PRINT("IO RELY ERROR : ModBus recv response failed\n");
        return -1;
    }

    for (i = 0; i < get_mb_info.n_byte; ++i)
    {
        set_mb_info.value[i] = get_mb_info.value[i] | (content->omask.up << (8 * i));
    }

    for (i = 0; i < get_mb_info.n_byte; ++i)
    {
        set_mb_info.value[i] &= ((~content->omask.down) << (8 * i));
    }

    /* send a modbsu request */
    if (0 > evoc_mb_send(mb_ctx, &set_mb_info))
    {
        MB_PRINT("IO RELY ERROR : ModBus send request failed\n");
        return -1;
    }

    /* recv a modbus response */
    if (0 > evoc_mb_recv(mb_ctx, &set_mb_info))
    {
        MB_PRINT("IO RELY ERROR : ModBus recv response failed\n");
        return -1;
    }

    return 0;
}

static void *io_rely_routine(void *arg)
{
    PTR_CHECK_NULL(arg);

    EVOCMB_CTX_T *mb_ctx = (EVOCMB_CTX_T *)arg;

    int i = 0;
    UINT64_T imask = 0;

    MB_INFO_T mb_info = {
        .code  = MB_FUNC_02,
        .reg   = 0x0000,
        .n_reg = 16
    };

    while (resource.running)
    {
        sleep(1);

        LOCK(&resource.lock);

        do 
        {
            if (!(resource.rely))
            {
                break;
            }

            /* send a modbsu request */
            if (0 > evoc_mb_send(mb_ctx, &mb_info))
            {
                MB_PRINT("RELY THREAD ERROR : ModBus send request failed\n");
                break;
            }

            /* recv a modbus response */
            if (0 > evoc_mb_recv(mb_ctx, &mb_info))
            {
                MB_PRINT("RELY THREAD ERROR : ModBus recv response failed\n");
                break;
            }

            imask = 0;

            for (i = 0; i < mb_info.n_byte; ++i)
            {
                imask = mb_info.value[i] << (i * 8);
            }

            mask_rule_macth(ruleset, imask, io_rely_handle, NULL);
        } while (0);

        ULOCK(&resource.lock);
    }

    return NULL;
}

static int resc_init(EVOCMB_CTX_T *mb_ctx)
{
    /* Create mutex lock */
    pthread_mutex_init(&(resource.lock), NULL);

    /* ModBus stay connected */
    if (0 > pthread_create(&(resource.pid[STAY_THREAD]), NULL, stay_connected_routine, mb_ctx))
    {
        perror("stay thread create error");
        return -1;
    }

    /* ModBus stay connected */
    if (0 > pthread_create(&(resource.pid[RELY_THREAD]), NULL, io_rely_routine, mb_ctx))
    {
        perror("rely thread create error");
        return -1;
    }

    return 0;
}

static void resc_uinit(void)
{
    int i = 0;

    /* wait thread finish */
    for (i = 0; i < THREAD_NUM; ++i)
    {
        pthread_join(resource.pid[i], NULL);
    }

    /* Create mutex lock */
    pthread_mutex_destroy(&(resource.lock));
}

int main(int argc, char *argv[ ])
{   
    EVOCMB_CTL_T ctl = default_ctl;

    arg_parse(argc, argv, &ctl);

    mb_ctx = evoc_mb_init(&ctl);
    if (!mb_ctx)
    {
        printf("Can not create evoc modbus context\n");
        return -1;
    }

    ruleset = mask_rule_init(MAX_MASK_RULE_NUM);
    if (!ruleset)
    {
        printf("Can not create mask rule set\n");
        return -1;
    }

    if (0 > resc_init(mb_ctx))
    {
        printf("Can not init evoc modbus resource\n");
        return -1;
    }

    /* evoc modbus work */
    work(mb_ctx);

    resc_uinit();

    mask_rule_exit(ruleset);

    evoc_mb_close(mb_ctx);
    
    printf("EVOC ModBus demo exit\n");

    return 0;
}
