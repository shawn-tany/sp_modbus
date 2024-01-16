#ifndef MNLT_COMMAND
#define MNLT_COMMAND

#include "list.h"

#define MAX_CMD_KEY_NUM (32)
#define MAX_CMD_KEY_LEN (32)

typedef int (*command_callback_func)(void *);

typedef struct 
{
    char cmd_key[MAX_CMD_KEY_LEN];
    int  offset;
} CMD_KEY_T;

typedef struct 
{
    struct list_head node;
    CMD_KEY_T cmd[MAX_CMD_KEY_NUM];
    int keynum;
    int keyoffset;
    int cmdid;
    command_callback_func callback_func;
} CMD_NODE_T;

typedef struct 
{
    struct list_head list;
    int cmdnum;
} CMD_LIST_T;

CMD_LIST_T *command_list_create(void);

int command_key_add(CMD_NODE_T *cmdnode, char *key, int keylen);

int command_node_add(CMD_LIST_T *cmdlist, CMD_NODE_T cmd);

void command_node_del(CMD_NODE_T *cmdnode);

void command_list_show(CMD_LIST_T *cmdlist);

void command_list_destory(CMD_LIST_T *cmdlist);

#define CMD_START                       \
{                                       \
    CMD_NODE_T node;                    \
    memset(&node, 0, sizeof(node));

#define CMD_LINE(key)                   \
    command_key_add(&node, key, strlen(key));

#define CMD_ADD(cmdlist, func)          \
    node.callback_func = func;          \
    command_node_add(cmdlist, node);

#define CMD_END                         \
}

#endif