#include <stdio.h>
#include <string.h>

#include "mask_rule.h"

int io_handle(MASK_RULE_CONTENT_T *content, void *arg)
{
    mask_rule_display(content);

    return 0;
}

int main()
{
    UINT64_T i = 0;
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
        addnode.content.type     = RULE_TYPE_FUZZ;

        if (i % 2)
        {
            addnode.content.imask.up   = i + 1;
            addnode.content.omask.up   = i + 1;
        }
        else
        {
            addnode.content.imask.down = i + 1;
            addnode.content.omask.down = i + 1;
        }

        mask_rule_add(ruleset, addnode);
    }

    for (i = 0; i < 100; ++i)
    {
        if (i % 2)
        {
            mask_rule_macth(ruleset, i + 1, io_handle, NULL);
        }
        else
        {
            mask_rule_macth(ruleset, ~(i + 1), io_handle, NULL);
        }
    }

    printf("----------------------------------------------\n");

    for (i = 0; i < 100; i += 2)
    {
        mask_rule_del(ruleset, i + 1);
    }

    for (i = 0; i < 100; ++i)
    {
        if (i % 2)
        {
            mask_rule_macth(ruleset, i + 1, io_handle, NULL);
        }
        else
        {
            mask_rule_macth(ruleset, ~(i + 1), io_handle, NULL);
        }
#if 0
        getnode = mask_rule_get(ruleset, i + 1);
        if (getnode)
        {
            io_handle(&getnode->content, NULL);
        }
#endif
    }

    mask_rule_exit(ruleset);

    return 0;
}