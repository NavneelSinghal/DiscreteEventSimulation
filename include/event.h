#ifndef EVENT_
#define EVENT_

#include "customer.h"
#include "teller.h"

typedef struct event_ {
	int type;
    customer* customer_var;
    teller* teller_var;
    double time;
    void (*action)(struct event_*);
} event;

typedef struct event_queue_node_ {
    event* e;
    struct event_queue_node_* next;
} event_node;

typedef struct {
    event_node *front, *back;
} event_queue;

event* new_event();
event_queue* new_event_queue();
void insert_event_ordered(event_queue* q, event* e);
int empty(event_queue* q);
event* top(event_queue* q);
void pop(event_queue* q);
void delete_event(event_queue* q, customer* c);

void free_event_queue(event_queue* q);

#endif
