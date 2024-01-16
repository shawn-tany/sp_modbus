#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "command.h"

CMD_LIST_T *command_list_create(void)
{
    CMD_LIST_T *cmdlist = NULL;

    cmdlist = (CMD_LIST_T *)malloc(sizeof(CMD_LIST_T));
    if (!cmdlist)
    {
        printf("Can not create command list : memory alloc failed !\n");
        return NULL;
    }

    INIT_LIST_HEAD(&cmdlist->list);

    cmdlist->cmdnum = 0;

    return cmdlist;
}

int command_key_add(CMD_NODE_T *cmdnode, char *key, int keylen)
{
    if (!cmdnode)
    {
        printf("Can not add key for command : invalid command node\n");
        return -1;
    }

    if (MAX_CMD_KEY_NUM <= cmdnode->keynum)
    {
        printf("Can not add key for command : too many key\n");
        return -1;
    }

    snprintf(cmdnode->cmd[cmdnode->keynum].cmd_key, MAX_CMD_KEY_LEN, "%s", key);
    cmdnode->cmd[cmdnode->keynum].offset = cmdnode->keyoffset;

    cmdnode->keynum++;
    cmdnode->keyoffset += (keylen + 1);

    return 0;
}

int command_node_add(CMD_LIST_T *cmdlist, CMD_NODE_T cmd)
{
    CMD_NODE_T *cmdnode = NULL;

    cmdnode = (CMD_NODE_T *)malloc(sizeof(CMD_NODE_T));
    if (!cmdnode)
    {
        printf("Can not create command node : memory alloc failed !\n");
        return -1;
    }

    memset(cmdnode, 0, sizeof(cmdnode));
    memcpy(cmdnode, &cmd, sizeof(cmd));

    list_add_tail(&cmdnode->node, &cmdlist->list);

    cmdnode->cmdid = ++cmdlist->cmdnum;

    return 0;
}

void command_node_del(CMD_NODE_T *cmdnode)
{
    list_del(&cmdnode->node);

    free(cmdnode);
}

void command_node_show(CMD_NODE_T *cmdnode)
{
    int i = 0;

    for (i = 0; i < cmdnode->keynum; ++i)
    {
        printf("%s ", cmdnode->cmd[i].cmd_key);
    }
    printf("\n");
}

void command_list_show(CMD_LIST_T *cmdlist)
{
    CMD_NODE_T *pos = NULL;
    CMD_NODE_T *tmp = NULL;

    list_for_each_entry_safe(pos, tmp, &cmdlist->list, node)
    {
        if (!pos)
        {
            continue;
        }

        command_node_show(pos);
    }
}

void command_list_destory(CMD_LIST_T *cmdlist)
{
    CMD_NODE_T *pos = NULL;
    CMD_NODE_T *tmp = NULL;

    list_for_each_entry_safe(pos, tmp, &cmdlist->list, node)
    {
        if (!pos)
        {
            continue;
        }

        command_node_del(pos);
    }

    free(cmdlist);
}
