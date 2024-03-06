#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"

#define PTR_CHECK(p)    \
{                       \
    if (!p)             \
    {                   \
        return -1;      \
    }                   \
}

dapp_stack_t *dapp_stack_create(int node_num, int node_size)
{
    dapp_stack_t *stack = NULL;

    stack = (dapp_stack_t *)malloc(sizeof(dapp_stack_t));
    if (!stack) 
    {
        return NULL;
    }

    stack->addr = malloc(node_num * node_size);
    if (!stack->addr) 
    {
        return NULL;
    }

    memset(stack->addr, 0, node_num * node_size);

    stack->top = stack->addr;
    stack->prod = stack->addr;

    stack->total = node_num;
    stack->avail = node_num;
    stack->offset = node_size;

    return stack;
}

int dapp_enstack(dapp_stack_t *stack, void *node, int size)
{
    PTR_CHECK(stack);
    PTR_CHECK(node);
    PTR_CHECK(stack->addr);

    if (size > stack->offset) 
    {
        return -2;
    }

    if (!stack->avail) 
    {
        return -3;
    }

    memcpy(stack->prod, node, size);

    if (stack->prod > stack->addr + (stack->offset * stack->total) ||
        stack->prod + stack->offset > stack->addr + (stack->offset * stack->total)) 
    {
        
        stack->prod = stack->addr;
    }

    stack->prod = stack->prod + stack->offset;
    
    stack->avail--;

    return 0;
}

int dapp_destack(dapp_stack_t *stack, void *node, int size)
{
    PTR_CHECK(stack);
    PTR_CHECK(node);
    PTR_CHECK(stack->addr);

    if (stack->avail == stack->total) 
    {
        return -4;
    }

    if (stack->offset < size) 
    {
        return -5;
    }

    stack->top = stack->prod - stack->offset;

    if (stack->top < stack->addr) 
    {
        stack->prod = stack->addr + ((stack->total - 1) * stack->offset);
    }

    memcpy(node, stack->top, size);
    
    stack->prod = stack->prod - stack->offset;
    
    stack->avail++;

    return 0;
}

int dapp_stack_top(dapp_stack_t *stack, void *node, int size)
{
    PTR_CHECK(stack);
    PTR_CHECK(node);
    PTR_CHECK(stack->addr);

    if (stack->avail == stack->total) 
    {
        return -4;
    }

    if (stack->offset < size) 
    {
        return -5;
    }

    void *top = NULL;

    top = stack->prod - stack->offset;

    memcpy(node, top, size);
    
    return 0;
}

void dapp_stack_free(dapp_stack_t *stack)
{
    if (stack) 
    {
        if (stack->addr) 
        {
            free(stack->addr);
            stack->addr = NULL;
        }

        stack->prod = NULL;
        stack->top = NULL;
        
        stack->total = 0;
        stack->avail = 0;
        stack->offset = 0;

        free(stack);
    }
}