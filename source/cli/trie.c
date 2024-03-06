#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "trie.h"

/* insert a new node, and update root node */
static int trie_new_node(TRIE_NODE_T **root, void *data, int size)
{
    int idx = (*root)->node_num;

    /* create new node */
    (*root)->node[idx] = (TRIE_NODE_T *)malloc(sizeof(TRIE_NODE_T) + size);
    if (!(*root)->node[idx])
    {
        return -1;
    }
    memset((*root)->node[idx], 0, sizeof(TRIE_NODE_T) + size);

    /* init new node */
    (*root)->node[idx]->data_len = size;
    memcpy((*root)->node[idx]->data, data, size);
    (*root)->node[idx]->node_num = 0;
    (*root)->node[idx]->depth = (*root)->depth + 1;
    
    /* updata search */
    (*root)->node_num++;
    (*root) = (*root)->node[idx];

    return 0;
}

int trie_strcmp(const void *src, int src_size, const void *dst, int dst_size)
{
    return (src_size == dst_size && !strncmp((char *)src, (char *)dst, src_size));
}

TRIE_ROOT_T *trie_init(trie_cmp_func_t cmp)
{
    TRIE_ROOT_T *root = NULL;

    /* create root node */
    root = (TRIE_ROOT_T *)malloc(sizeof(*root));
    if (!root)
    {
        return NULL;
    }
    memset(root, 0, sizeof(*root));

    root->cmp = cmp;
    root->root.depth = 0;

    return root;
}

int trie_insert(TRIE_ROOT_T *root, void **pattern, int *pattern_len, int ele_num, trie_entry_handle_func_t func)
{
    int pattern_idx = 0;
    int node_idx    = 0;
    int search_flag = 0;

    TRIE_NODE_T *search = &root->root;

    for (pattern_idx = 0; pattern_idx < ele_num; ++pattern_idx)
    {
        /* new node */
        if (!search->node_num)
        {
            if (0 > trie_new_node(&search, pattern[pattern_idx], pattern_len[pattern_idx]))
            {
                printf("create new node failed\n");
                return -1;
            }
            root->total_node_num++;
            continue;
        }

        search_flag = 0;

        /* search node */
        for (node_idx = 0; node_idx < search->node_num; ++node_idx)
        {
            /* exist */
            if (root->cmp(pattern[pattern_idx], pattern_len[pattern_idx], 
                            search->node[node_idx]->data, search->node[node_idx]->data_len))
            {
                /* updata search */
                search = search->node[node_idx];
                search_flag = 1;
                break;                
            }
        }

        /* not found */
        if (!search_flag)
        {
            if (0 > trie_new_node(&search, pattern[pattern_idx], pattern_len[pattern_idx]))
            {
                printf("create new node failed\n");
                return -1;
            }
            root->total_node_num++;
        }
    }

    /* conffict */
    if (search->tail_flag)
    {
        printf("trie path conffict\n");
        return -1;
    }

    search->tail_flag = 1;
    search->func      = func;

    return 0;
}

TRIE_NODE_T *tire_found(TRIE_ROOT_T *root, void **pattern, int *pattern_len, int ele_num)
{
    int pattern_idx = 0;
    int node_idx    = 0;
    int search_flag = 0;

    TRIE_NODE_T *search = &root->root;

    for (pattern_idx = 0; pattern_idx < ele_num; ++pattern_idx)
    {
        /* not any node */
        if (!search->node_num)
        {
            return NULL;
        }

        search_flag = 0;

        /* search node */
        for (node_idx = 0; node_idx < search->node_num; ++node_idx)
        {
            /* exist */
            if (root->cmp(pattern[pattern_idx], pattern_len[pattern_idx], 
                            search->node[node_idx]->data, search->node[node_idx]->data_len))
            {
                /* updata search */
                search = search->node[node_idx];
                search_flag = 1;
                break;                
            }
        }

        /* not found */
        if (!search_flag)
        {
            return NULL;
        }
    }

    if (!search->tail_flag)
    {
        return NULL;
    }

    return search;
}

int trie_child_entry(TRIE_NODE_T *root, trie_entry_handle_func_t func)
{
    int child_idx = 0; 

    for (child_idx = 0; child_idx < root->node_num; ++child_idx)
    {
        if (root->node[child_idx])
        {
            if (!func)
            {
                continue;
            }

            if (0 > func(root->node[child_idx]->data, root->node[child_idx]->data_len))
            {
                return -1;
            }
        }
    }

    return 0;
}

int trie_path_entry(TRIE_ROOT_T *root, trie_entry_handle_func_t func)
{
    int i   = 0;
    int ret = 0;

    dapp_stack_t *stack = NULL;
    TRIE_NODE_T  *node  = NULL;

    stack = dapp_stack_create(root->total_node_num, sizeof(TRIE_NODE_T *));
    if (!stack)
    {
        printf("create stack failed\n");
        return -1;
    }

    for (i = 0; i < root->root.node_num; ++i)
    {
        if (!root->root.node[i])
        {
            break;
        }

        dapp_enstack(stack, (void *)(&root->root.node[i]), sizeof(TRIE_NODE_T *));
    }

    while (!dapp_destack(stack, (void *)(&node), sizeof(TRIE_NODE_T *)))
    {
        if (node->node_num)
        {
            for (i = 0; i < node->node_num; ++i)
            {
                dapp_enstack(stack, (void *)(&node->node[i]), sizeof(TRIE_NODE_T *));
            }
        }

        if (!node->tail_flag)
        {
            continue;
        }

        if (func)
        {
            if (0 > func(node->data, node->data_len))
            {
                ret = -1;
                break;
            }
        }
    }

    dapp_stack_free(stack);

    return ret;
}

int trie_path_list(TRIE_ROOT_T *root, trie_path_handle_func_t func)
{
    int i   = 0;
    int ret = 0;

    dapp_stack_t *stack = NULL;
    TRIE_NODE_T  *node  = NULL;

    void *child_data[MAX_CHILD_DEPTH];
    int   child_data_length[MAX_CHILD_DEPTH] = {0};
    int   child_index[MAX_CHILD_DEPTH] = {0};
    int   depth = 0;
    int   idx = 0;

    stack = dapp_stack_create(root->total_node_num, sizeof(TRIE_NODE_T *));
    if (!stack)
    {
        printf("create stack failed\n");
        return -1;
    }

    for (i = 0; i < root->root.node_num; ++i)
    {
        if (!root->root.node[i])
        {
            break;
        }

        dapp_enstack(stack, (void *)(&root->root.node[i]), sizeof(TRIE_NODE_T *));
    }

    while (!dapp_stack_top(stack, (void *)(&node), sizeof(TRIE_NODE_T *)))
    {
        depth = node->depth;

        child_data[depth - 1] = node->data;
        child_data_length[depth - 1] = node->data_len;

        idx = child_index[depth]++;

        if (node->node_num && idx < node->node_num)
        {
            dapp_enstack(stack, (void *)(&node->node[idx]), sizeof(TRIE_NODE_T *));
            continue;
        }
        else
        {
            dapp_destack(stack, (void *)(&node), sizeof(TRIE_NODE_T *));
            child_index[depth] = 0;
        }

        if (!node->tail_flag || !func)
        {
            continue;
        }

        if (0 > func((const void **)child_data, child_data_length, depth))
        {
            ret = -1;
            break;
        }
    }

    dapp_stack_free(stack);

    return ret;
}

int trie_each_entry_accord_hierarchy(TRIE_ROOT_T *root, trie_entry_handle_func_t func)
{
    int i   = 0;
    int ret = 0;

    dapp_stack_t *stack = NULL;
    TRIE_NODE_T  *node  = NULL;

    stack = dapp_stack_create(root->total_node_num, sizeof(TRIE_NODE_T *));
    if (!stack)
    {
        printf("create stack failed\n");
        return -1;
    }

    for (i = 0; i < root->root.node_num; ++i)
    {
        if (!root->root.node[i])
        {
            break;
        }

        dapp_enstack(stack, (void *)(&root->root.node[i]), sizeof(TRIE_NODE_T *));
    }

    while (!dapp_destack(stack, (void *)(&node), sizeof(TRIE_NODE_T *)))
    {
        if (node->node_num)
        {
            for (i = 0; i < node->node_num; ++i)
            {
                dapp_enstack(stack, (void *)(&node->node[i]), sizeof(TRIE_NODE_T *));
            }
        }

        if (!func)
        {
            continue;
        }

        if (0 > func(node->data, node->data_len))
        {
            ret = -1;
            break;
        }
    }

    dapp_stack_free(stack);

    return ret;
}

int trie_uinit(TRIE_ROOT_T *root)
{
    int i = 0;

    dapp_stack_t *stack = NULL;
    TRIE_NODE_T  *node  = NULL;

    stack = dapp_stack_create(root->total_node_num, sizeof(TRIE_NODE_T *));
    if (!stack)
    {
        printf("create stack failed\n");
        return -1;
    }

    for (i = 0; i < root->root.node_num; ++i)
    {
        if (!root->root.node[i])
        {
            break;
        }

        dapp_enstack(stack, (void *)(&root->root.node[i]), sizeof(TRIE_NODE_T *));
    }

    while (!dapp_destack(stack, (void *)(&node), sizeof(TRIE_NODE_T *)))
    {
        if (node->node_num)
        {
            for (i = 0; i < node->node_num; ++i)
            {
                dapp_enstack(stack, (void *)(&node->node[i]), sizeof(TRIE_NODE_T *));
            }
        }

        /* free node */
        free(node);
    }

    free(root);

    dapp_stack_free(stack);

    return 0;
}

#if 1

static struct 
{
    char *pattern[32];
    int   pattern_num;
} pattern_list[] = 
{
    {{"hello", "world"}, 2},
    {{"this", "is", "my", "trie"}, 4},
    {{"this", "is", "my"}, 3},
    {{"hello", "worldd"}, 2},
    {{"hello", "worldd", "hhh"}, 3},
    {{"hello", "worldd", "ggg"}, 3},
    {{"hello", "worldd", "ggg", "hhh"}, 4},
    {{"this", "is"}, 2},
    {{"this", "is", "my", "test"}, 4},
    {{"i", "want", "so", "much", "money"}, 5},
    {{"wu", "qi", "biao", "da", "shuai", "bi"}, 6},
    {{"niu", "le", "xiao", "diao", "mao"}, 5}
};

static struct 
{
    char *pattern[32];
    int   pattern_num;
} invalid_pattern_list[] = 
{
    {{"hello", "firend"}, 2},
    {{"this", "is", "your", "trie"}, 4},
    {{"this", "is", "your"}, 3},
    {{"hello", "firendd"}, 2},
    {{"hello", "firendd", "hhh"}, 3},
    {{"hello", "firendd", "ggg"}, 3},
    {{"hello", "firendd", "ggg", "hhh"}, 4},
    {{"that", "is"}, 2},
    {{"that", "is", "my", "test"}, 4},
    {{"i", "need", "so", "much", "money"}, 5},
    {{"wu", "qi", "da", "shuai", "bi"}, 4},
    {{"niu", "le", "diao", "mao"}, 4}
};

int trie_entry_print(const void *data, int data_len)
{
    printf("%s\n", (char *)data);
}

int trie_path_print(const void **data_array, const int *data_len_array, const int item_num)
{
    int i = 0;

    for (i = 0; i < item_num; ++i)
    {
        printf("%s ", (const char *)data_array[i]);
    }

    printf("\n");

    return 0;
}

int main()
{
    int i = 0;
    int j = 0;
    TRIE_ROOT_T *root = NULL;
    TRIE_NODE_T *node = NULL;

    int pattern_len[32] = {0};

    root = trie_init(trie_strcmp);
    if (!root)
    {
        printf("trie init failed\n");
        return -1;
    }

    for (i = 0; i < TRIE_ITEM(pattern_list); ++i)
    {
        for (j = 0; j < pattern_list[i].pattern_num; ++j)
        {
            pattern_len[j] = strlen(pattern_list[i].pattern[j]) + 1;
        }
        trie_insert(root, (void **)pattern_list[i].pattern, pattern_len, pattern_list[i].pattern_num, NULL);
    }

    for (i = 0; i < TRIE_ITEM(pattern_list); ++i)
    {
        for (j = 0; j < pattern_list[i].pattern_num; ++j)
        {
            pattern_len[j] = strlen(pattern_list[i].pattern[j]) + 1;
        }
        if ((node = tire_found(root, (void **)pattern_list[i].pattern, pattern_len, pattern_list[i].pattern_num)))
        {
            printf("search pattern%d\n", i + 1);
        }
    }

    for (i = 0; i < TRIE_ITEM(invalid_pattern_list); ++i)
    {
        for (j = 0; j < invalid_pattern_list[i].pattern_num; ++j)
        {
            pattern_len[j] = strlen(invalid_pattern_list[i].pattern[j]) + 1;
        }
        if ((node = tire_found(root, (void **)invalid_pattern_list[i].pattern, pattern_len, invalid_pattern_list[i].pattern_num)))
        {
            printf("search invalid pattern%d\n", i + 1);
        }
    }

    trie_path_entry(root, trie_entry_print);

    printf("-------------------------------\n");

    trie_path_list(root, trie_path_print);

    printf("-------------------------------\n");

    trie_each_entry_accord_hierarchy(root, trie_entry_print);

    trie_uinit(root);

    return 0;
}
#endif