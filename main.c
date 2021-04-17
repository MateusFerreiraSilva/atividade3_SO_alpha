#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include "queue.h"

#define END 10000

struct shared_area
{
    sem_t mutex;
    pid_t pid[7];
    int pNum;
    int counter;
    int turn; // turn 0->P5, 1->P6, 2->P7T1, 3->P7T2, 4->P7T3
    int qntVal[2];
    short freq[1001];
    queue f1, f2;
};

typedef struct shared_area shared_area;

shared_area *shared_area_ptr;
sem_t thread_mutex;
int canal1[2], canal2[2];
int MEM_SZ = sizeof(shared_area);

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
            if (!queue_empty(q) && shared_area_ptr->counter < END)
            {
                val = queue_pop(q);
                printf("%d\n", val);
                fflush(stdout);
                shared_area_ptr->freq[val]++;
                shared_area_ptr->counter++;
            }
            if (shared_area_ptr->counter == END)
                break;
            next_turn();
        }
    }
    next_turn();
    pthread_exit(NULL);
}

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
                write(canal1[1], &val, sizeof(int));
            else
                write(canal2[1], &val, sizeof(int));
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
        r = rand() % 1001;
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
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC, &begin);

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
    fflush(stdout);

    if (shared_memory == (void *)-1)
    {
        printf("shmat falhou\n");
        exit(-1);
    }

    shared_memory = shmat(shmid, (void *)0, 0);

    printf("Memoria compartilhada no endereco=%p\n", shared_memory);
    fflush(stdout);

    shared_area_ptr = (shared_area *)shared_memory;

    // semaforo inicia fechado
    if (sem_init((sem_t *)&shared_area_ptr->mutex, 1, 0) != 0)
    {
        printf("sem_init falhou\n");
        exit(-1);
    }

    // Inicalizacoes
    int pid[7], status[7];
    memset(shared_area_ptr->pid, -1, sizeof(shared_area_ptr->pid));
    memset(shared_area_ptr->qntVal, 0, sizeof(shared_area_ptr->qntVal));
    memset(shared_area_ptr->freq, 0, sizeof(shared_area_ptr->freq));
    shared_area_ptr->pNum = 7;
    shared_area_ptr->counter = 0;
    shared_area_ptr->turn = -1; // deixa p5, p6 e p7 parados
    queue_init(&shared_area_ptr->f2);
    queue_init(&shared_area_ptr->f1);
    if (pipe(canal1) == -1 || pipe(canal2) == -1)
    {
        printf("Erro pipe()");
        exit(-1);
    }

    for (int i = 0; i < shared_area_ptr->pNum; i++)
    {
        pid[i] = fork();
        if (pid[i] == 0)
        { // filhos
            shared_area_ptr->pid[i] = getpid();

            switch (i)
            {

            case 0: // p1
            case 1: // p2
            case 2: // p3
            {
                fechaCanais();
                f1Gerenete();
            }

            case 3: // p4
            {

                // fecha canais de leitura
                close(canal1[0]);
                close(canal2[0]);

                signal(SIGUSR1, waitSignal);
                while (1)
                    pause(); // espera pelo sinal
            }

            case 4: // p5
            case 5: // p6
            {
                // fecha canais de escrita
                close(canal1[1]);
                close(canal2[1]);

                queue *q = &shared_area_ptr->f2;
                int turn = i == 4 ? 0 : 1;
                int val;

                // checa se pipe esta fechado

                while (1)
                { // f2Produce
                    if (shared_area_ptr->turn == turn)
                    {
                        ssize_t bytes_read =
                            turn == 0 ? read(canal1[0], &val, sizeof(int))
                                      : read(canal2[0], &val, sizeof(int));

                        if (bytes_read == sizeof(int))
                            if (!queue_full(q))
                                if (queue_push(q, val))
                                    shared_area_ptr->qntVal[turn]++;

                        next_turn();
                    }
                }
            }

            case 6: // p7
            {
                fechaCanais();
                pthread_t t[3];
                const int turn[3] = {2, 3, 4};
                for (int i = 0; i < 3; i++)
                    pthread_create(&t[i], NULL, f2Consume, (void *)&turn[i]);
                for (int i = 0; i < 3; i++)
                    pthread_join(t[i], NULL);
                exit(status[i]);
            }

            } // switch
        }     // if
    }         // for
    fechaCanais();

    sleep(2);
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

    pid[6] = wait(&status[6]);  // espera p7 terminar
    for (int i = 0; i < 6; i++) // finaliza todos os outros filhos
        kill(pid[i], SIGKILL);

    for (int i = 0; i < shared_area_ptr->pNum; i++)
        pid[i] = wait(&status[i]);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double seconds = end.tv_sec - begin.tv_sec;
    // double nanoseconds = end.tv_nsec - begin.tv_nsec / 1000000000.0;
    // seconds += nanoseconds;
    double minutes = seconds / 60;

    printf("\nTempo total de execução:\n");
    printf("%.2lf segundos ou %.2lf minutos\n\n", seconds, minutes);

    printf("Quantidade de valores processados:\n");
    printf("P5 %d\n", shared_area_ptr->qntVal[0]);
    printf("P6 %d\n\n", shared_area_ptr->qntVal[1]);

    int moda[2] = {0}, maxVal = 0;
    for (int i = 1; i < 1000; i++)
    {
        if (moda[0] < shared_area_ptr->freq[i])
        {
            moda[0] = shared_area_ptr->freq[i];
            moda[1] = i;
        }
        if (shared_area_ptr->freq[i] > 0 && maxVal < i)
            maxVal = i;
    }

    printf("Moda: %d\n", moda[1]);
    printf("Maior valor: %d\n", maxVal);

    return 0;
}