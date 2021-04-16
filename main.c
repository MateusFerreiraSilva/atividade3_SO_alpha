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

#define MEM_SZ 4096
#define BUFF_SZ MEM_SZ - sizeof(int) - sizeof(sem_t)

struct shared_area
{
    sem_t mutex;
    int pid[7];
    queue f1;
};

typedef struct shared_area shared_area;

shared_area *shared_area_ptr;
sem_t thread_mutex;

void *randConsume(void *args)
{
    int id = *((int *)args);
    queue *q = &shared_area_ptr->f1;
    int val;
    while (1)
    {
        sem_wait((sem_t *)&thread_mutex);
        if (!queue_empty(q))
        {
            val = queue_pop(q);
            printf("-->%d -->t%d\n", val, id);
        }
        sem_post((sem_t *)&thread_mutex);

        if (queue_empty(q))
            break;
    }
    pthread_exit(NULL);
}

void randConsumeUtil(int signal) // p2
{
    pthread_t t[2];
    const int id[2] = {1, 2};
    if (sem_init((sem_t *)&thread_mutex, 0, 1) != 0)
    {
        printf("sem_init thread falhou\n");
        exit(-1);
    }

    sem_wait((sem_t *)&shared_area_ptr->mutex);
    for (int i = 0; i < 2; i++)
        pthread_create(&t[i], NULL, randConsume, (void *)&id[i]);
    for (int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);
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
                kill(shared_area_ptr->pid[3], SIGUSR1);
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

    // semaforo inicia fechado
    if (sem_init((sem_t *)&shared_area_ptr->mutex, 1, 0) != 0)
    {
        printf("sem_init falhou\n");
        exit(-1);
    }

    queue_init(&shared_area_ptr->f1);
    int pid[7], status[7];
    memset(shared_area_ptr->pid, -1, sizeof shared_area_ptr->pid);

    for (int i = 0; i < 4; i++)
    {
        pid[i] = fork();
        if (pid[i] == 0)
        { // filhos
            shared_area_ptr->pid[i] = getpid();
            if (i < 3)
            { // p1 ou p2 ou p3
                // printf("Processo %d pid %d %d\n", i + 1, getpid(), shared_area_ptr->pid[i]);
                randGerenete();
            }
            else if (i < 4)
            { // p4
                // printf("Processo %d pid %d %d\n", i + 1, getpid(), shared_area_ptr->pid[i]);
                signal(SIGUSR1, randConsumeUtil);
                while (1)
                    pause(); // espera pelo sinal
            }
        }
    }

    int counter;
    while (1) // espera todos os processo inicializarem
    {
        counter = 0;
        for (int i = 0; i < 4; i++)
            if (shared_area_ptr->pid[i] != -1)
                counter++;
        if (counter == 4)
        {
            sem_post((sem_t *)&shared_area_ptr->mutex); // abre o semaforo para liberar p1, p2, p3 e p4
            break;
        }
        else
            sleep(1);
    }

    for (int i = 0; i < 4; i++)
        pid[i] = wait(&status[i]);

    return 0;
}