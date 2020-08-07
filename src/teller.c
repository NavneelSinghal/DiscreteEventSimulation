#include "../include/teller.h"

#include <stdlib.h>

// now implement all functions of a teller and teller_queue

teller* new_teller() {
    teller* t = (teller*)malloc(sizeof(teller));
    t->customer_queue_var = NULL;
    t->teller_number = -1;
    t->service_time = 0.0;
    t->state = 0;
    return t;
}

double total_time_in_queue(teller* t) {
    double tm = 0.0;
    customer_queue* q = t->customer_queue_var;
    customer_node* n = q->front;
    while (n != NULL) {
        tm += n->customer_var->service_time;
        n = n->next;
    }
    return tm;
}
