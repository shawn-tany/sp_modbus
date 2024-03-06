#ifndef MNLT_CLI
#define MNLT_CLI

#include <string.h>
#include "trie.h"

#define ITEM(a) (sizeof(a) / sizeof(a[0]))

#define CMD_START                       \
{                                       \
    char *pattern[MAX_CHILD_NUM];       \
    int   pattern_len[MAX_CHILD_NUM];   \
    int   pattern_idx = 0;

#define CMD_LINE(subpattern)            \
    pattern[pattern_idx] = subpattern;  \
    pattern_len[pattern_idx] = strlen(subpattern);\
    pattern_idx++;


#define CMD_ADD(trie, func)             \
    trie_insert(trie, (void **)pattern, pattern_len, pattern_idx, func);

#define CMD_END                         \
}

#endif