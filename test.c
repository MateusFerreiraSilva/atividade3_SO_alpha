#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include "queue.h"
#include <semaphore.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

int main()
{
    int n, c1 = 0, c2 = 0;
    while (scanf("%d", &n) != EOF)
    {
        if (n == 0)
            c1++;
        else if (n == 1)
            c2++;
    }

    printf("%d %d\n", c1, c2);
}