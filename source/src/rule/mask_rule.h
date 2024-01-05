#ifndef MASK_RULE
#define MASK_RULE

#include "list.h"

/* Base type define */
typedef unsigned char       UINT8_T;
typedef unsigned short      UINT16_T;
typedef unsigned int        UINT32_T;
typedef unsigned long long  UINT64_T;

#define MASK_RULE_PRIORITY_NUM 8

typedef struct 
{
    UINT8_T  io_stat;
    UINT64_T i_mask;
    UINT64_T o_mask;
} MASK_RULE_CONTENT_T;;

typedef struct 
{
    struct list_head id_head;
    struct list_head priority_head;

    UINT16_T rule_id;
    UINT8_T priority;
    UINT8_T valid;
    MASK_RULE_CONTENT_T content;
} MASK_RULE_NODE_T;

typedef struct 
{
    struct list_head  list;
    UINT8_T           rule_num;
    MASK_RULE_NODE_T *last_node;
} MASK_RULE_LIST_T;

typedef struct 
{
    UINT16_T rule_cnt;
    UINT16_T rule_cap;
    MASK_RULE_LIST_T id_list;
    MASK_RULE_LIST_T priority_list[MASK_RULE_PRIORITY_NUM];
    char rule_mem[0];
} MASK_RULE_T;

typedef int (*match_callback)(MASK_RULE_CONTENT_T *);

MASK_RULE_T *mask_rule_init(UINT16_T rule_num);

int mask_rule_exit(MASK_RULE_T *ruleset);

int mask_rule_add(MASK_RULE_T *ruleset, MASK_RULE_NODE_T *rulenode);

int mask_rule_del(MASK_RULE_T *ruleset, UINT8_T rule_id);

MASK_RULE_NODE_T *mask_rule_get(MASK_RULE_T *ruleset, UINT8_T rule_id);

int mask_rule_macth(MASK_RULE_T *ruleset, UINT64_T mask_rule, match_callback func);

#endif