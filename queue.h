#define SUCCESS 1001010
#define FAIL -1001010

typedef struct node node;
typedef struct queue queue;

node *node_create(int val);
void free_node(node *no);
queue *queue_create(int maxSize);
void free_queue(queue *q);
int queue_size(queue *q);
int queue_empty(queue *q);
int queue_full(queue *q);
int queue_front(queue *q);
int queue_back(queue *q);
int queue_insert(queue *q, int val);
int queue_pop(queue *q);
void queue_print(queue *q);