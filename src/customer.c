#include "../include/customer.h"

#include <assert.h>
#include <stdlib.h>

customer* new_customer() {
    customer* c = (customer*)malloc(sizeof(customer));
    c->arrival_time = 0.0;
    c->teller_number = -1;  // default value for unassigned teller
    c->service_time = 0.0;
    c->service_timestamp = 0.0;
    return c;
}

customer_queue* new_customer_queue() {
    customer_queue* q = (customer_queue*)malloc(sizeof(customer_queue));
    q->front = NULL;
    q->back = NULL;
    q->size = 0;
    return q;
}

int customer_queue_length(customer_queue* q) { return q->size; }

customer_node* new_customer_node(customer* c) {
    customer_node* n = (customer_node*)malloc(sizeof(customer_node));
    n->customer_var = c;
    n->next = NULL;
    return n;
}

void customer_push(customer_queue* q, customer* c) {
    customer_node* n = new_customer_node(c);
    if (q->back == NULL) {
        assert(q->front == NULL);
        q->front = n;
        q->back = n;
    } else {
        assert(q->back != NULL);
        assert(q->front != NULL);
        q->back->next = n;
        q->back = n;
    }
    q->size++;
}

void customer_pop(customer_queue* q) {
    if (q->front != NULL) {
        customer_node* oldfront = q->front;
        q->front = oldfront->next;
        if (q->front == NULL) {
            q->back = NULL;
        }
        free(oldfront);
        q->size--;
    }
}

void free_customer_queue(customer_queue* q) {
    customer_node* n = q->front;
    while (n != NULL) {
        customer_node* nextnode = n->next;
        free(n->customer_var);
        free(n);
        n = nextnode;
    }
}
