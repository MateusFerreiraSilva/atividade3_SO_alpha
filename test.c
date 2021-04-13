#include <stdio.h>
#include "queue.h"

int main()
{
    queue *q = queue_create(10);
    int val = queue_pop(q);
    queue_print(q);
    queue_insert(q, 500);
    queue_print(q);
}