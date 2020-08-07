#include <stdlib.h>
#include "../include/event.h"

//implement all the functions of an event

event* new_event(){
    event* e = (event*) malloc(sizeof(event));
    e->type = 0;
    e->customer_var = NULL;
    e->teller_var = NULL;
    e->time = 0;
    return e;
}

event_node* new_event_node(event* e){
    event_node* n = (event_node*) malloc(sizeof(event_node));
    n->e = e;
    n->next = NULL;
    return n;
}

event_queue* new_event_queue(){
    event_queue* q = (event_queue*) malloc(sizeof(event_queue));
    q->front = NULL;
    q->back = NULL;
    return q;
}

void insert_event_ordered(event_queue* q, event* e){
    event_node* n = new_event_node(e);
    if(q->front == NULL){
        q->front = n;
        q->back = n;
    }
    else{
        if(q->front->e->time > n->e->time){
            n->next = q->front;
            q->front = n;
        }
        else if(q->back->e->time <= n->e->time){
            q->back->next = n;
            q->back = n;
        }
        else{
            event_node* start = q->front;
            event_node* prev = NULL;
            while(start->e->time <= n->e->time){
                prev = start;
                start = start->next;
            }
            n->next = prev->next;
            prev->next = n;
        }
    }
}

int empty(event_queue* q){
    return (q->front == NULL);
}

event* top(event_queue* q){
    if(q->front != NULL){
        return q->front->e;
    }
    else return NULL;
}

void pop(event_queue* q){
    if(q->front != NULL){
        event_node* oldfront = q->front;
        q->front = oldfront->next;
        if(q->front == NULL){
            q->back = NULL;
        }
        free(oldfront);
    }
}

void delete_node(event_queue* q, event_node* n){
    if(q->front == n){
        pop(q);
    }
    else if(q->back == n){
        event_node* n1 = q->front;
        event_node* prev = NULL;
        while(n1->next != NULL){
            prev = n1;
            n1 = n1->next;
        }
        free(q->back);
        q->back = prev;
    }
    else{
        event_node* h = q->front;
        event_node* prev = NULL;
        while(h != n && h != NULL){
            prev = h;
            h = h->next;
        }
        if(h != NULL){
            prev->next = h->next;
            free(h);
        }
    }
}

void delete_event(event_queue* q, customer* c){
    event_node* n = q->front;
    while(n != NULL){
        if(n->e->customer_var == c){
            delete_node(q, n);
        }
        n = n->next;
    }
}

void free_event_queue(event_queue* q){
    event_node* n = q->front;
    while(n != NULL){
        event_node* p = n->next;
        free(n->e);
        free(n);
        n = p;
    }
}
