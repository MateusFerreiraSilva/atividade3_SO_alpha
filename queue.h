#define SUCCESS 10101
#define FAIL 0
#define QUEUE_MAX_SIZE 10

struct queue
{
    int front, rear, size;
    int data[QUEUE_MAX_SIZE];
};

typedef struct queue queue;

// void queue_init(queue *q);
// int queue_front(queue *q);
// int queue_rear(queue *q);
// int queue_size(queue *q);
// int queue_full(queue *q);
// int queue_empty(queue *q);
// int queue_push(queue *q, int val);
// int queue_pop(queue *q);
// void queue_print(queue *q);

void queue_init(queue *q)
{
    q->front = 0;
    q->rear = QUEUE_MAX_SIZE - 1;
    q->size = 0;
}

int queue_front(queue *q)
{
    return q->data[q->front];
}

int queue_rear(queue *q)
{
    return q->data[q->rear];
}

int queue_size(queue *q)
{
    return q->size;
}

int queue_full(queue *q)
{
    return q->size == QUEUE_MAX_SIZE;
}

int queue_empty(queue *q)
{
    return q->size == 0;
}

int queue_push(queue *q, int val)
{
    if (queue_full(q))
        return FAIL;

    q->rear = (q->rear + 1) % QUEUE_MAX_SIZE;
    q->data[q->rear] = val;
    q->size++;

    return SUCCESS;
}

int queue_pop(queue *q)
{
    if (queue_empty(q))
        return FAIL;

    int val = q->data[q->front];
    q->front = (q->front + 1) % QUEUE_MAX_SIZE;
    q->size--;
    return val;
}
