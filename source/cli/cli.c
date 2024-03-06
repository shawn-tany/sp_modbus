#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>

#include "trie.h"
#include "cli.h"

static struct 
{
    TRIE_ROOT_T *root;
    TRIE_NODE_T *search;

    int index;
    int start;
    int end;
} cmd_ctx;

char *command_generator(const char *text, int state)
{
    int length = strlen(text);
    int i = cmd_ctx.index;
    char *retstr = NULL;

    if (cmd_ctx.search->node_num <= i)
    {
        printf("%s : %d : %s\n", __FUNCTION__, __LINE__, cmd_ctx.search->data);
        goto NO_SUCH;
    }

    if (!cmd_ctx.search->node[i]->data_len)
    {
        printf("%s : %d : %s\n", __FUNCTION__, __LINE__, cmd_ctx.search->data);
        goto NO_SUCH;
    }

    for (; i < cmd_ctx.search->node_num; ++i)
    {
        if (!strncmp(cmd_ctx.search->node[i]->data, text, length))
        {
            if (length)
            {
                retstr = cmd_ctx.search->node[i]->data;
                cmd_ctx.search = cmd_ctx.search->node[i];
                cmd_ctx.index = 0;
            }
            else 
            {
                retstr = cmd_ctx.search->node[i]->data;
                cmd_ctx.index++;
            }

            return strdup(retstr);
        }
    }

NO_SUCH :

    cmd_ctx.index = 0;

    return ((char *)NULL);
}

char **mlt_completion(const char *text, int start, int end)
{
    char **matches;

    matches = (char **)NULL;

    cmd_ctx.start = start;
    cmd_ctx.end   = end;

    if (0 == start)
    {
        cmd_ctx.search = &cmd_ctx.root->root;
    }

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
    cmd_ctx.root   = NULL;
    cmd_ctx.search = NULL;
    cmd_ctx.index  = 0;
    cmd_ctx.start  = 0;
    cmd_ctx.end    = 0;

    cmd_ctx.root = trie_init(trie_strcmp);
    if (!cmd_ctx.root)
    {
        printf("failed to create command trie\n");
        return -1;
    }

    cmd_ctx.search = &cmd_ctx.root->root;

    CMD_START;
    CMD_LINE("exit");
    CMD_ADD(cmd_ctx.root, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("rely");
    CMD_LINE("on");
    CMD_ADD(cmd_ctx.root, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("rely");
    CMD_LINE("off");
    CMD_ADD(cmd_ctx.root, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("stay");
    CMD_LINE("on");
    CMD_ADD(cmd_ctx.root, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("stay");
    CMD_LINE("off");
    CMD_ADD(cmd_ctx.root, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("funcode");
    CMD_LINE("1");
    CMD_LINE("reg");
    CMD_LINE("0x0000-0x0010");
    CMD_LINE("nreg");
    CMD_LINE("1-16");
    CMD_ADD(cmd_ctx.root, NULL);
    CMD_END;

    CMD_START;
    CMD_LINE("funcode");
    CMD_LINE("2");
    CMD_LINE("reg");
    CMD_LINE("0x0010");
    CMD_ADD(cmd_ctx.root, NULL);
    CMD_END;
}

int main(int argc, char **argv)
{
    char *line;

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
    
    return 0;
}
