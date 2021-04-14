#define SUCCESS 1001010
#define FAIL -1001010
#define QUEUE_MAX_SIZE 10

typedef struct queue queue;

void queue_init(queue *q);
int queue_front(queue *q);
int queue_rear(queue *q);
int queue_size(queue *q);
int queue_full(queue *q);
int queue_empty(queue *q);
int queue_push(queue *q, int val);
int queue_pop(queue *q);
void queue_print(queue *q);
