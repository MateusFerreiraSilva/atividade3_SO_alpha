#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include "queue.h"
#include <semaphore.h>
#include <signal.h>

#define MEM_SZ 4096
#define BUFF_SZ MEM_SZ - sizeof(int) - sizeof(sem_t)

struct shared_area
{
    sem_t mutex;
    int p4Id;
    queue f1;
};

typedef struct shared_area shared_area;

shared_area *shared_area_ptr;

void randConsume(int signal)
{
    queue *q = &shared_area_ptr->f1;
    int val;
    sem_wait((sem_t *)&shared_area_ptr->mutex);
    while (queue_size(q) > 0)
    {
        val = queue_pop(q);
        printf("-->%d\n", val);
    }
    sem_post((sem_t *)&shared_area_ptr->mutex);
}

void randGerenete()
{
    int id = getpid(), r;
    queue *q = &shared_area_ptr->f1;
    while (1)
    {
        r = rand() % 1000 + 1;
        sem_wait((sem_t *)&shared_area_ptr->mutex);
        if (!queue_full(q))
        {
            queue_push(q, r);
            printf("%d - Processo: %d Escreveu %d\n", queue_size(q), id, r);
            if (queue_full(q))
            {
                printf("Processo %d deu o sinal!!!\n", id);
                kill(shared_area_ptr->p4Id, SIGUSR1);
            }
        }
        sem_post((sem_t *)&shared_area_ptr->mutex);
    }
}

int main()
{
    key_t key = 1101;
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

    shared_area_ptr = (shared_area *)shared_memory;

    if (sem_init((sem_t *)&shared_area_ptr->mutex, 1, 1) != 0)
    {
        printf("sem_init falhou\n");
        exit(-1);
    }

    queue_init(&shared_area_ptr->f1);
    int pid[7], status[7];

    pid[3] = fork(); // cria p4
    if (pid[3] == 0)
    { // p4
        signal(SIGUSR1, randConsume);
        while (1)
        {
            pause(); // espera pelo sinal
        }
    }
    else // pai
        shared_area_ptr->p4Id = pid[3];

    for (int i = 0; i < 3; i++) // cria p1, p2, p3
    {
        pid[i] = fork();
        if (pid[i] == 0) // p1 ou p2 ou p3
            randGerenete();
    }

    for (int i = 0; i < 4; i++)
        pid[i] = wait(&status[i]);

    return 0;
}