#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>

#include "command.h"

static struct 
{
    CMD_LIST_T *cmdlist;
    CMD_NODE_T *cmdnode;

    int start;
    int end;
    int finish;
} cmd_ctx;

char *command_generator(const char *text, int state)
{
    int length = strlen(text);
    int i = 0;
    CMD_NODE_T *lastnode = list_last_entry(&cmd_ctx.cmdlist->list, CMD_NODE_T, node);

    if (cmd_ctx.finish)
    { 
        return ((char *)NULL);
    }

    do
    {
        if (!cmd_ctx.cmdnode)
        {
            cmd_ctx.cmdnode = list_first_entry(&cmd_ctx.cmdlist->list, CMD_NODE_T, node);
        }                                               
        else
        {
            cmd_ctx.cmdnode = list_next_entry(cmd_ctx.cmdnode, node);
        }

        if (cmd_ctx.cmdnode->cmdid >= cmd_ctx.cmdlist->cmdnum)
        {
            cmd_ctx.finish = 1;
        }

        for (i = 0; i < cmd_ctx.cmdnode->keynum; i++)
        {
            if (cmd_ctx.cmdnode->cmd[i].offset != cmd_ctx.start)
            {
                continue;
            }

            if (!strncmp(cmd_ctx.cmdnode->cmd[i].cmd_key, text, length))
            {
                return strdup(cmd_ctx.cmdnode->cmd[i].cmd_key);
            }
        }

    } while (!cmd_ctx.finish);

    return ((char *)NULL);
}

char **mlt_completion(const char *text, int start, int end)
{
    char **matches;

    matches = (char **)NULL;

    cmd_ctx.start  = start;
    cmd_ctx.end    = end;
    cmd_ctx.finish = 0;

    matches = rl_completion_matches(text, command_generator);

    return (matches);
}

void initialize_readline()
{
    chdir("/workdir");
    
    rl_readline_name = ">";
    
    rl_attempted_completion_function = mlt_completion;
}

int command_ctx_init(void)
{
    cmd_ctx.cmdlist = NULL;
    cmd_ctx.cmdnode = NULL;

    cmd_ctx.start = 0;
    cmd_ctx.end   = 0;

    cmd_ctx.cmdlist = command_list_create();
    if (!cmd_ctx.cmdlist)
    {
        printf("failed to create command list\n");
        return -1;
    }

    CMD_START;
    CMD_LINE("exit");
    CMD_ADD(cmd_ctx.cmdlist, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("rely");
    CMD_LINE("on");
    CMD_ADD(cmd_ctx.cmdlist, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("rely");
    CMD_LINE("off");
    CMD_ADD(cmd_ctx.cmdlist, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("stay");
    CMD_LINE("on");
    CMD_ADD(cmd_ctx.cmdlist, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("stay");
    CMD_LINE("off");
    CMD_ADD(cmd_ctx.cmdlist, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("funcode");
    CMD_LINE("1");
    CMD_LINE("reg");
    CMD_LINE("0x0000-0x0010");
    CMD_LINE("nreg");
    CMD_LINE("1-16");
    CMD_ADD(cmd_ctx.cmdlist, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("funcode");
    CMD_LINE("2");
    CMD_LINE("reg");
    CMD_LINE("0x0010");
    CMD_ADD(cmd_ctx.cmdlist, NULL);
    CMD_END;

    command_list_show(cmd_ctx.cmdlist);
}

void command_ctx_uinit(void)
{
    command_list_destory(cmd_ctx.cmdlist);
}

int main(int argc, char **argv)
{
    char *line, *s;

    command_ctx_init();

    initialize_readline();

    while (1)
    {
        line = readline("mlt@cli:~$ ");

        if (!line)
        {
            break;
        }

        if (strlen(line))
        {
            add_history(line);
        }
        
        free(line);
    }

    command_ctx_uinit();
    
    return 0;
}
