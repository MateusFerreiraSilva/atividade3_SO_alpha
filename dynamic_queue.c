#include <stdio.h>
#include <stdlib.h>
#include "dynamic_queue.h"

struct node
{
    int val;
    struct node *nxt;
};

struct queue
{
    struct node *front;
    struct node *back;
    int size;
    int maxSize;
};

node *node_create(int val)
{
    node *no = malloc(sizeof(node));
    if (no)
    {
        no->val = val;
        no->nxt = NULL;
    }
}

void free_node(node *no)
{
    free(no);
}

queue *queue_create(int maxSize)
{
    queue *q = malloc(sizeof(queue));
    if (q)
    {
        q->front = NULL;
        q->back = NULL;
        q->size = 0;
        q->maxSize = maxSize;
    }
    return q;
}

void free_queue(queue *q)
{
    free(q);
}

int queue_size(queue *q)
{
    return q->size;
}

int queue_empty(queue *q)
{
    return q->size == 0;
}

int queue_full(queue *q)
{
    return q->size == q->maxSize;
}

int queue_front(queue *q)
{
    if (q->size > 0)
        return q->front->val;
    return FAIL;
}

int queue_back(queue *q)
{
    if (q->size > 0)
        return q->back->val;
    return FAIL;
}

int queue_insert(queue *q, int val)
{
    if (queue_full(q))
        return FAIL;

    node *no = node_create(val);
    if (q->size > 0)
    {
        no->nxt = q->back;
        q->back = no;
    }
    else
    {
        q->front = no;
        no->nxt = NULL;
        q->back = no;
    }
    q->size++;
}

int queue_pop(queue *q)
{
    if (queue_empty(q))
        return FAIL;

    int val = q->front->val;
    if (q->size > 1)
    {
        printf("Entrou!\n");
        node *no = q->back;
        while (no->nxt->val != q->front->val)
            no = no->nxt;

        free_node(q->front);
        no->nxt = NULL;
        q->front = no;
    }
    else
    {
        free_node(q->front);
        q->front = NULL;
        q->back = NULL;
    }
    q->size--;
    return val;
}

void queue_print(queue *q)
{
    node *no = q->back;
    while (no)
    {
        printf("%d -> ", no->val);
        no = no->nxt;
    }
    printf("\n");
}