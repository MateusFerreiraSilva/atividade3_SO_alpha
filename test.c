#include <stdio.h>
#include <pthread.h>
#include "queue.h"

void *func(void *args)
{
    int *val = args;
    printf("%d\n", *val);
    pthread_exit(NULL);
}

int main()
{
    while (1)
    {
    }
    pthread_t t[2];
    const int val[2] = {40, 45};
    for (int i = 0; i < 2; i++)
    {
        pthread_create(&t[i], NULL, func, (void *)&val[i]);
    }

    for (int i = 0; i < 2; i++)
    {
        pthread_join(t[i], NULL);
    }
}