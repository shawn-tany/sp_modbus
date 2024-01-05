#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mask_rule.h"

MASK_RULE_T *mask_rule_init(UINT16_T rule_num)
{
    int i = 0;
    UINT32_T size = sizeof(MASK_RULE_T) + (sizeof(MASK_RULE_NODE_T) * rule_num);
    MASK_RULE_NODE_T *node = NULL;

    MASK_RULE_T *ruleset = (MASK_RULE_T *)malloc(size);
    if (!ruleset)
    {
        return NULL;
    }
    memset(ruleset, 0, size);

    ruleset->rule_cap = rule_num;

    for (i = 0; i < ruleset->rule_cap; ++i)
    {
        node = ((MASK_RULE_NODE_T *)ruleset->rule_mem) + i;
        list_add(&(ruleset->id_list.list), &node->id_head);
    }

    return ruleset;
}

int mask_rule_exit(MASK_RULE_T *ruleset)
{
    free(ruleset);

    return 0;
}

int mask_rule_add(MASK_RULE_T *ruleset, MASK_RULE_NODE_T *rulenode)
{
    if (MASK_RULE_PRIORITY_NUM <= rulenode->priority)
    {
        return -1;
    }

    if (ruleset->rule_cnt >= ruleset->rule_cap)
    {
        return -1;
    }

    rulenode->valid   = 1;
    rulenode->rule_id = ruleset->rule_cnt;

    list_add(&(ruleset->priority_list[rulenode->priority].list), &rulenode->priority_head);

    ruleset->rule_cnt++;

    return 0;
}

int mask_rule_del(MASK_RULE_T *ruleset, UINT8_T rule_id)
{
    MASK_RULE_NODE_T *rulenode;
    MASK_RULE_NODE_T *pos;

    list_for_each_entry_safe(pos, rulenode, &ruleset->id_list.list, id_head)
    {
        if (rulenode->rule_id == rule_id)
        {
            rulenode->rule_id  = 0;
            rulenode->valid    = 0;
            rulenode->priority = 0;

            list_del(&rulenode->priority_head);

            list_del(&rulenode->id_head);
            list_add(&ruleset->id_list.list, &rulenode->id_head);
        }
    }

    return 0;
}

MASK_RULE_NODE_T *mask_rule_get(MASK_RULE_T *ruleset, UINT8_T rule_id)
{
    MASK_RULE_NODE_T *rulenode;
    MASK_RULE_NODE_T *pos;

    list_for_each_entry_safe(pos, rulenode, &ruleset->id_list.list, id_head)
    {
        if (rulenode->rule_id == rule_id)
        {
            return rulenode;
        }
    }

    return NULL;
}

int mask_rule_macth(MASK_RULE_T *ruleset, UINT64_T mask_rule, match_callback func)
{
    int i = 0;

    MASK_RULE_NODE_T *rulenode;
    MASK_RULE_NODE_T *pos;

    for (i = 0; i < MASK_RULE_PRIORITY_NUM; ++i)
    {
        list_for_each_entry_safe(pos, rulenode, &ruleset->id_list.list, id_head)
        {
            if (mask_rule == rulenode->content.i_mask)
            {
                return func(&rulenode->content);
            }
        }
    }

    return -1;
}