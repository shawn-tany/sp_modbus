#include <stdio.h>
#include <string.h>

#include "mask_rule.h"

int io_handle(MASK_RULE_CONTENT_T *content, void *arg)
{
    printf("id(%d) iMask(0x%llx) oMask(0x%llx) ioStat(%s)\n", content->rule_id, 
        content->i_mask, content->o_mask, content->io_stat ? "up" : "down");
}

int main()
{
    int i = 0;
    MASK_RULE_T      *ruleset = NULL;
    MASK_RULE_NODE_T  addnode;
    MASK_RULE_NODE_T *getnode = NULL;

    ruleset = mask_rule_init(100);
    if (!ruleset)
    {
        printf("can not create rule set\n");
        return -1;
    }

    for (i = 0; i < 100; ++i)
    {
        memset(&addnode, 0, sizeof(addnode));

        addnode.content.priority = i % 8;
        addnode.content.i_mask   = i;
        addnode.content.o_mask   = i;
        addnode.content.io_stat  = i % 2;

        mask_rule_add(ruleset, addnode);
    }

    for (i = 0; i < 100; ++i)
    {
        mask_rule_macth(ruleset, i, io_handle, NULL);
    }

    printf("----------------------------------------------\n");

    for (i = 0; i < 100; i += 2)
    {
        mask_rule_del(ruleset, i + 1);
    }

    for (i = 0; i < 100; ++i)
    {
        mask_rule_macth(ruleset, i, io_handle, NULL);
    }

    mask_rule_exit(ruleset);

    return 0;
}