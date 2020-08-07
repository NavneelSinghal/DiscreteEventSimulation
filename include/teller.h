#ifndef TELLER_
#define TELLER_

#include "customer.h"

typedef struct {
    customer_queue* customer_queue_var;
    int teller_number;
    double service_time;
    int state;
} teller;

teller* new_teller();
double total_time_in_queue(teller* t);

#endif
