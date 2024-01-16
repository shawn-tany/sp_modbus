#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mask_rule.h"

enum mask_rule_err 
{
    MR_SUCCESS = 0,
    MR_ERR_MEM_ALLOC,
    MR_ERR_CAP_FULL,
    MR_ERR_CAP_EMPTY,
    MR_ERR_INVALID_PRIO,
    MR_ERR_INVALID_TYPE,
    MR_ERR_INVALID_IMASK,
    MR_ERR_INVALID_OMASK,
    MR_ERR_GET_HEAD,
    MR_ERR_GET_NODE,
    MR_ERR_SEARCH_NODE,
    MR_ERR_INVALID_NODE,
    MR_ERR_CALLBACK,
};

char *mask_rule_err_get(int err)
{
    struct
    {
        int   code;
        char *msg;
    } err_map[] =
    {
        { MR_SUCCESS,            "success"                    },
        { -MR_ERR_MEM_ALLOC,     "failed to create memory"    },
        { -MR_ERR_CAP_FULL,      "rule capacity is full"      },
        { -MR_ERR_CAP_EMPTY,     "rule capacity is empty"     },
        { -MR_ERR_INVALID_PRIO,  "invalid priority"           },
        { -MR_ERR_INVALID_TYPE,  "invalid rule type"          },
        { -MR_ERR_INVALID_IMASK, "invalid imask"              },
        { -MR_ERR_INVALID_OMASK, "invalid omask"              },
        { -MR_ERR_GET_HEAD,      "failed to get list head"    },
        { -MR_ERR_GET_NODE,      "failed to get rule node"    },
        { -MR_ERR_SEARCH_NODE,   "failed to search rule node" },
        { -MR_ERR_INVALID_NODE,  "invalid rule node"          },
        { -MR_ERR_CALLBACK,      "callback error"             }
    };

    int i = 0;

    for (i = MR_SUCCESS; i <= MR_ITEM(err_map); ++i)
    {
        if (err == err_map[i].code)
        {
            return err_map[i].msg;
        }
    }

    return "unkown";
}

MASK_RULE_T *mask_rule_init(UINT16_T rule_num)
{
    int i = 0;

    /* rule set size */
    UINT32_T size = sizeof(MASK_RULE_T) + (sizeof(MASK_RULE_NODE_T) * rule_num);
    MASK_RULE_NODE_T *node = NULL;

    /* create a rule set */
    MASK_RULE_T *ruleset = (MASK_RULE_T *)malloc(size);
    if (!ruleset)
    {
        return NULL;
    }
    memset(ruleset, 0, size);

    ruleset->rule_cap = rule_num;

    /* init id list */
    INIT_LIST_HEAD(&(ruleset->id_list.list));

    /* updata id list */
    for (i = 0; i < ruleset->rule_cap; ++i)
    {
        node = ((MASK_RULE_NODE_T *)ruleset->rule_mem) + i;
        list_add(&node->id_head, &(ruleset->id_list.list));
    }

    /* updata last id node */
    ruleset->id_list.last_node = &ruleset->id_list.list;

    for (i = 0; i < MASK_RULE_PRIORITY_NUM; ++i)
    {
        /* init priority list */
        INIT_LIST_HEAD(&ruleset->priority_list[i].list);

        /* updata last priority node */
        ruleset->priority_list[i].last_node = &ruleset->priority_list[i].list;
    }

    return ruleset;
}

void mask_rule_exit(MASK_RULE_T *ruleset)
{
    if (ruleset)
    {
        free(ruleset);
    }
}

int mask_rule_add(MASK_RULE_T *ruleset, MASK_RULE_NODE_T rulenode)
{
    int priority = 0;
    int ruletype = rulenode.content.type;
    int prioidx  = 0;

    struct list_head *newhead     = NULL;
    MASK_RULE_NODE_T *newrulenode = NULL;

    /* check cap */
    if (ruleset->rule_cnt >= ruleset->rule_cap)
    {
        return -MR_ERR_CAP_FULL;
    }

    /* check priority */
    if (MASK_RULE_PRIORITY_NUM < priority || 0 > priority)
    {
        return -MR_ERR_INVALID_PRIO;
    }

    /* check type */
    if (RULE_TYPE_FLEX != ruletype && RULE_TYPE_FUZZ != ruletype)
    {
        return -MR_ERR_INVALID_TYPE;
    }

    /* check imask */
    if (rulenode.content.imask.down & rulenode.content.imask.up)
    {
        return -MR_ERR_INVALID_IMASK;
    }

    /* check omask */
    if (rulenode.content.omask.down & rulenode.content.omask.up)
    {
        return -MR_ERR_INVALID_OMASK;
    }

    /* get a empty node head from id list */
    newhead = ruleset->id_list.last_node->next;
    if (!newhead)
    {
        return -MR_ERR_GET_HEAD;
    }

    /* get a empty node from id list */
    newrulenode = list_entry(newhead, MASK_RULE_NODE_T, id_head);
    if (!newrulenode)
    {
        return -MR_ERR_GET_NODE;
    }

    /* updata rule node */
    newrulenode->content         = rulenode.content;
    newrulenode->valid           = 1;
    newrulenode->content.rule_id = ++(ruleset->rule_id);
    if (!newrulenode->content.priority)
    {
        newrulenode->content.priority = MASK_RULE_PRIORITY_NUM - 1;
    }

    priority = newrulenode->content.priority;
    prioidx  = priority ? (priority - 1) : 0;

    /* inset a new to priority list tail */
    list_add_tail(&newrulenode->priority_head, ruleset->priority_list[prioidx].last_node);

    /* updata last id node */
    ruleset->id_list.last_node = &newrulenode->id_head;

    /* updata last priority node */
    ruleset->priority_list[prioidx].last_node = &newrulenode->priority_head;

    /* updata rule count */
    ruleset->priority_list[prioidx].rule_num++;
    ruleset->rule_cnt++;

    return MR_SUCCESS;
}

int mask_rule_del(MASK_RULE_T *ruleset, UINT8_T rule_id)
{
    int priority = 0;
    MASK_RULE_NODE_T *rulenode;

    /* empty rule set */
    if (!ruleset->rule_cnt)
    {
        return -MR_ERR_CAP_EMPTY;
    }

    rulenode = mask_rule_get(ruleset, rule_id);

    if (!rulenode)
    {
        return -MR_ERR_SEARCH_NODE;
    }

    priority = rulenode->content.priority;

    if (ruleset->id_list.last_node == &ruleset->id_list.list &&
        ruleset->priority_list[priority].last_node == &ruleset->priority_list[priority].list)
    {
        return -1;
    }

    /* updata last id node */
    if (ruleset->id_list.last_node == &rulenode->id_head)
    {
        ruleset->id_list.last_node = rulenode->id_head.prev;
    }

    /* updata last priority node */
    if (ruleset->priority_list[priority].last_node == &rulenode->id_head)
    {
        ruleset->priority_list[priority].last_node = rulenode->id_head.prev;
    }

    /* reset rule node */
    rulenode->valid              = 0;
    rulenode->content.imask.up   = 0;
    rulenode->content.imask.down = 0;
    rulenode->content.omask.up   = 0;
    rulenode->content.omask.down = 0;
    rulenode->content.priority   = 0;
    rulenode->content.rule_id    = 0;

    /* delete rule node from priority list */
    list_del(&rulenode->priority_head);

    /* updata rule id list */
    list_del(&rulenode->id_head);
    list_add_tail(&rulenode->id_head, &ruleset->id_list.list);

    /* updata rule count */
    ruleset->rule_cnt--;
    ruleset->priority_list[priority].rule_num--;

    return MR_SUCCESS;
}

MASK_RULE_NODE_T *mask_rule_get(MASK_RULE_T *ruleset, UINT8_T rule_id)
{
    MASK_RULE_NODE_T *tmp;
    MASK_RULE_NODE_T *pos;

    /* empty rule set */
    if (!ruleset->rule_cnt)
    {
        return NULL;
    }

    /* search each rule node */
    list_for_each_entry_safe(pos, tmp, &ruleset->id_list.list, id_head)
    {
        if (!pos)
        {
            return NULL;
        }

        if (!pos->valid)
        {
            return NULL;
        }

        /* search by rule id */
        if (pos->content.rule_id == rule_id)
        {
            return pos;
        }
    }

    return NULL;
}

static int mask_rule_fitler(MASK_RULE_T *ruleset, MASK_RULE_NODE_T *filter)
{
    if (!ruleset || !filter)
    {
        return 0;
    }

    int i = 0;
    MASK_RULE_NODE_T *tmp = NULL;
    MASK_RULE_NODE_T *pos = NULL;

    /* empty rule set */
    if (!ruleset->rule_cnt)
    {
        return -MR_ERR_CAP_EMPTY;
    }

    for (i = 0; i < MASK_RULE_PRIORITY_NUM; ++i)
    {
        list_for_each_entry_safe(pos, tmp, &ruleset->priority_list[i].list, priority_head)
        {
            if (!pos)
            {
                return 0;
            }

            if (!pos->valid)
            {
                return 0;
            }

            if (!pos->effect)
            {
                continue;
            }

            if (filter->content.priority < pos->content.priority)
            {
                return 0;
            }
            else if (filter->content.priority == pos->content.priority)
            {
                if (filter->content.rule_id <= pos->content.rule_id)
                {
                    return 0;
                }
            }

            if (filter->content.omask.up & pos->content.omask.down ||
                filter->content.omask.down & pos->content.omask.up)
            {
                return 1;
            }
        }
    }

    return 0;
}

int mask_rule_macth(MASK_RULE_T *ruleset, UINT64_T mask_rule, match_callback func, void *arg)
{
    int i   = 0;
    UINT64_T upmask = 0;
    UINT64_T downmask = 0;
    MASK_RULE_NODE_T *tmp = NULL;
    MASK_RULE_NODE_T *pos = NULL;

    /* empty rule set */
    if (!ruleset->rule_cnt)
    {
        return -MR_ERR_CAP_EMPTY;
    }

    for (i = 0; i < MASK_RULE_PRIORITY_NUM; ++i)
    {
        list_for_each_entry_safe(pos, tmp, &ruleset->priority_list[i].list, priority_head)
        {
            if (!pos)
            {
                return -MR_ERR_GET_NODE;
            }

            if (!pos->valid)
            {
                return -MR_ERR_INVALID_NODE;
            }

            pos->effect = 0;

            if (pos->content.type == RULE_TYPE_FLEX)
            {
                upmask   = (mask_rule);
                downmask = ((~mask_rule));
            }
            else if (pos->content.type == RULE_TYPE_FUZZ)
            {
                upmask   = (mask_rule & pos->content.imask.up);
                downmask = ((~mask_rule) & pos->content.imask.down);
            }

            if (pos->content.imask.up && upmask != pos->content.imask.up)
            {
                continue;
            }

            if (pos->content.imask.down && (downmask != pos->content.imask.down))
            {
                continue;
            }

            if (mask_rule_fitler(ruleset, pos))
            {
                continue;
            }

            pos->effect = 1;

            if (0 > func(&pos->content, arg))
            {
                return -MR_ERR_CALLBACK;
            }
        }
    }

    return MR_SUCCESS;
}

void mask_rule_display(const MASK_RULE_CONTENT_T *content)
{
    printf("id(%d)", content->rule_id);

    if (content->imask.up)
    {
        printf(" imask.up(0x%llx)", content->imask.up);
    }

    if (content->imask.down)
    {
        printf(" imask.down(0x%llx)", content->imask.down);
    }
    
    if (content->omask.up)
    {
        printf(" omask.up(0x%llx)", content->omask.up);
    }

    if (content->omask.down)
    {
        printf(" omask.down(0x%llx)", content->omask.down);
    }

    printf(" priority(%d)\n", content->priority);
}