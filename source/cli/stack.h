#ifndef DAPP_STACK
#define DAPP_STACK

typedef struct 
{
    void *addr;
    void *top;
    void *prod;
    int offset;
    int avail;
    int total;
} dapp_stack_t;

dapp_stack_t *dapp_stack_create(int node_num, int node_size);

int dapp_enstack(dapp_stack_t *stack, void *node, int size);

int dapp_destack(dapp_stack_t *stack, void *node, int size);

int dapp_stack_top(dapp_stack_t *stack, void *node, int size);

void dapp_stack_free(dapp_stack_t *stack);

#endif
