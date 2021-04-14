#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include "queue.h"
#include <semaphore.h>

#define MEM_SZ 4096
#define BUFF_SZ MEM_SZ - sizeof(int) - sizeof(sem_t)

struct shared_area
{
    sem_t mutex;
    queue f1;
};

typedef struct shared_area shared_area;

void randGerenete(shared_area *shared_area_ptr, int id)
{
    int r;
    queue *q = &shared_area_ptr->f1;
    while (1)
    {
        r = rand() % 1000 + 1;
        sem_wait((sem_t *)&shared_area_ptr->mutex);
        // printf("Processo %d Escreveu %d\n", id, r);
        if (!queue_full(q))
        {
            queue_push(q, r);
            printf("Processo %d Escreveu %d\n", id, r);
        }
        sem_post((sem_t *)&shared_area_ptr->mutex);
    }
}

int main()
{
    key_t key = 1101;
    struct shared_area *shared_area_ptr;
    void *shared_memory = (void *)0;
    int shmid;

    shmid = shmget(key, MEM_SZ, 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        printf("shmget falhou\n");
        exit(-1);
    }

    printf("shmid=%d\n", shmid);

    if (shared_memory == (void *)-1)
    {
        printf("shmat falhou\n");
        exit(-1);
    }

    shared_memory = shmat(shmid, (void *)0, 0);

    printf("Memoria compartilhada no endereco=%p\n", shared_memory);

    shared_area_ptr = (struct shared_area *)shared_memory;

    if (sem_init((sem_t *)&shared_area_ptr->mutex, 1, 1) != 0)
    {
        printf("sem_init falhou\n");
        exit(-1);
    }

    queue_init(&shared_area_ptr->f1);

    int pids[4], status[4];

    for (int i = 0; i < 4; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0) // filho
        {
            if (i < 3)
                randGerenete(shared_area_ptr, getpid());
            else
                i = i - i + i;
            exit(status[i]);
        }
    }

    for (int i = 0; i < 4; i++)
    {
        pids[i] = wait(&status[i]);
    }

    return 0;
}