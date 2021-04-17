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
#include <fcntl.h>

#define MEM_SZ 4096

struct shared_area
{
    sem_t mutex;
    int pid[7];
    int pNum, counter;
    int turn; // turn 0->P5, 1->P6, 2->P7T1, 3->P7T2, 4->P7T3
    queue f1, f2;
};

typedef struct shared_area shared_area;

shared_area *shared_area_ptr;
sem_t thread_mutex;
int canal1[2], canal2[2];

void fechaCanais()
{
    close(canal1[1]);
    close(canal1[1]);
    close(canal2[0]);
    close(canal2[0]);
}

void next_turn()
{
    shared_area_ptr->turn = (shared_area_ptr->turn + 1) % 5;
}

void *f2Consume(void *args)
{
    int turn = *((int *)args);
    queue *q = &shared_area_ptr->f2;
    int val;
    while (1)
    {
        if (shared_area_ptr->turn == turn)
        {
            if (!queue_empty(q) && shared_area_ptr->counter < 1000)
            {
                val = queue_pop(q);
                printf("%d\n", val);
                fflush(stdout);
                shared_area_ptr->counter++;
            }
            if (shared_area_ptr->counter == 1000)
                break;
            next_turn();
        }
    }
    next_turn();
    pthread_exit(NULL);
}

// void f2Produce(int val, int turn)
// {
//     queue *q = &shared_area_ptr->f2;
//     while (1)
//     {
//         if (shared_area_ptr->turn == turn)
//         {
//             if (!queue_full(q))
//             {
//                 printf("val %d turn %d\n", val, turn);
//                 fflush(stdout);
//                 queue_push(q, val);
//             }
//             next_turn();
//         }
//     }
// }

void *f1Consume(void *args)
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
            if (id == 1)
            {
                write(canal1[1], &val, sizeof(int));
            }
            else
            {
                write(canal2[1], &val, sizeof(int));
            }
        }
        sem_post((sem_t *)&thread_mutex);

        if (queue_empty(q))
            break;
    }
    sem_post((sem_t *)&shared_area_ptr->mutex);
}

void waitSignal()
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
        pthread_create(&t[i], NULL, f1Consume, (void *)&id[i]);
    for (int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);

    sem_post((sem_t *)&shared_area_ptr->mutex);
}

void f1Gerenete()
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
            if (queue_full(q))
                kill(shared_area_ptr->pid[3], SIGUSR1);
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

    // Inicalizacoes!!!!!!!!
    queue_init(&shared_area_ptr->f1);
    queue_init(&shared_area_ptr->f2);
    int pid[7], status[7];
    memset(shared_area_ptr->pid, -1, sizeof shared_area_ptr->pid);
    shared_area_ptr->pNum = 7;
    shared_area_ptr->counter = 0;
    shared_area_ptr->turn = -1; // deixa p5, p6 e p7 parados
    if (pipe(canal1) == -1 || pipe(canal2) == -1)
    {
        printf("Erro pipe()");
        exit(-1);
    }
    // else if (fcntl(canal1[0], F_SETFL, O_NONBLOCK) < 0 || fcntl(canal2[0], F_SETFL, O_NONBLOCK) < 0)
    // {
    //     printf("Erro pipe()");
    //     exit(-1);
    // }

    fflush(stdout);
    for (int i = 0; i < shared_area_ptr->pNum; i++)
    {
        pid[i] = fork();
        if (pid[i] == 0)
        { // filhos
            shared_area_ptr->pid[i] = getpid();
            if (i < 3)
            { // p1 ou p2 ou p3
                fechaCanais();
                f1Gerenete();
            }
            else if (i < 4)
            { // p4

                // fecha canais de leitura
                close(canal1[0]);
                close(canal2[0]);

                signal(SIGUSR1, waitSignal);
                while (1)
                    pause(); // espera pelo sinal
            }
            else if (i < 6)
            { // p5 e p6
                // fecha canais de escrita
                close(canal1[1]);
                close(canal2[1]);

                queue *q = &shared_area_ptr->f2;
                int turn = i == 4 ? 0 : 1;
                int val;

                // checa se pipe esta fechado

                if (turn == 0)
                {
                    while (1)
                    {
                        if (shared_area_ptr->turn == turn)
                        {
                            if (read(canal1[0], &val, sizeof(int)) > 0)
                            {
                                // printf("read %d turn %d\n", val, turn);
                                // fflush(stdout);
                                if (!queue_full(q))
                                {
                                    val = queue_push(q, val);
                                    // printf("%d\n", queue_rear(q));
                                    // fflush(stdout);
                                }
                            }
                            next_turn();
                        }
                    }
                }

                else if (turn == 1)
                {
                    while (1)
                    {
                        if (shared_area_ptr->turn == turn)
                        {
                            if (read(canal2[0], &val, sizeof(int)) > 0)
                            {
                                // printf("read %d turn %d\n", val, turn);
                                // fflush(stdout);
                                if (!queue_full(q))
                                {
                                    val = queue_push(q, val);
                                    // printf("%d\n", queue_rear(q));
                                    // fflush(stdout);
                                }
                            }
                            next_turn();
                        }
                    }
                }
            }
            else
            { // p7
                fechaCanais();
                pthread_t t[3];
                const int turn[3] = {2, 3, 4};
                for (int i = 0; i < 3; i++)
                    pthread_create(&t[i], NULL, f2Consume, (void *)&turn[i]);
                for (int i = 0; i < 3; i++)
                    pthread_join(t[i], NULL);

                printf("counter %d\n", shared_area_ptr->counter);
                exit(status[i]);
            }
        }
    }

    int counter;
    while (1) // espera todos os processo inicializarem
    {
        counter = 0;
        for (int i = 0; i < shared_area_ptr->pNum; i++)
            if (shared_area_ptr->pid[i] != -1)
                counter++;
        if (counter == shared_area_ptr->pNum)
        {
            sem_post((sem_t *)&shared_area_ptr->mutex); // abre o semaforo para liberar p1, p2, p3 e p4
            shared_area_ptr->turn = 0;
            break;
        }
        else
            sleep(1);
    }

    for (int i = 0; i < shared_area_ptr->pNum; i++)
        pid[i] = wait(&status[i]);

    return 0;
}