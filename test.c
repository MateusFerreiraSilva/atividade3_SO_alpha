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
    int p[2];
    pipe(p);
    int pid, status;
    pid = fork();
    if (pid == 0)
    {
        close(p[0]);
        int i = 0;
        while (1)
        {
            write(p[1], &i, sizeof(int));
            // printf("write %d\n", i);
            i++;
        }
    }
    else
    {
        close(p[1]);

        int i;
        while (1)
        {
            read(p[0], &i, sizeof(int));
            printf("read %d\n", i);
            fflush(stdout);
            // buffer[aux++] = i;
            // if (aux == 1000)
            // {
            //     for (int j = 0; j < 1000; j++)
            //     {
            //         printf("read %d\n", buffer[j]);
            //     }
            //     aux = 0;
            // }
        }

        pid = wait(&status);
    }
}