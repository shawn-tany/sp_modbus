#ifndef SP_STACK
#define SP_STACK

typedef struct 
{
    void *addr;
    void *top;
    void *prod;
    int offset;
    int avail;
    int total;
} SP_STACK_T;

SP_STACK_T *sp_stack_create(int node_num, int node_size);

int sp_enstack(SP_STACK_T *stack, void *node, int size);

int sp_destack(SP_STACK_T *stack, void *node, int size);

int sp_stack_top(SP_STACK_T *stack, void *node, int size);

void sp_stack_free(SP_STACK_T *stack);

#endif
