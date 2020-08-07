#ifndef CUSTOMER_
#define CUSTOMER_

typedef struct {
    double arrival_time;
    int teller_number;
    double service_time;
    double service_timestamp;
    int customer_index;
} customer;

typedef struct customer_queue_node_ {
    customer* customer_var;
    struct customer_queue_node_* next;
} customer_node;

typedef struct {
    customer_node *front, *back;
    int size;
} customer_queue;

customer* new_customer();
customer_queue* new_customer_queue();
int customer_queue_length(customer_queue* q);
void customer_push(customer_queue* q, customer* c);
void customer_pop(customer_queue* q);
void free_customer_queue(customer_queue* q);

#endif
