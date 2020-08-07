#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include "../include/event.h"
#include "../include/customer.h"
#include "../include/teller.h"

#define MINUTES_TO_SEC 60
#define ARRIVAL 1
#define SERVICE 2
#define DEPARTURE 3
#define WAKE 4
#define MAX_IDLE_TIME 150
#define INF 1e9
#define EPS 1e-4

#define DEBUG
#ifdef DEBUG
#define dbg_printf printf
#else
#define dbg_printf(...) 1
#endif

int mode;
int start;

int num_customers;
int num_tellers;
double sim_time;
double avg_service_time;

double time_now;
int customers_processed;
double total_time_for_all_customers;
double* time_spent_in_bank;
double* wait_time_till_teller;
double* teller_service_time;
double* arrival_times;
double* raw_arrival_times;
double* teller_service_times;
int* random_teller;
int cur_teller;

int func_pointer_use;

event_queue* q_e;
teller** t;
int* used;
double* wake_times;

FILE* gnuplotpipe;
FILE* datafile;

void customer_arrival(event*);
void customer_service(event*);
void customer_departure(event*);
void teller_wake_up(event*);

char* names[4] = {"ARRIVAL", "SERVICE", "DEPARTURE", "WAKE"};

void print_customer(customer* c){
    dbg_printf("Printing customer details\n");
    dbg_printf("Index: %d\nArrival time: %f\nTeller number: %d\nService time taken: %f\nService timestamp: %f\n", c->customer_index, c->arrival_time, c->teller_number, c->service_time, c->service_timestamp);
}

void print_teller(teller* te){
    dbg_printf("Printing teller details\n");
    dbg_printf("Index: %d\nService time: %f\n", te->teller_number, te->service_time);
}

void print_all(){
    for(int i = 0; i < num_tellers; i++){
        dbg_printf("Length of teller %d's queue is %d\n", i, customer_queue_length(t[i]->customer_queue_var));
    }
}

void print_event(event* e){
    dbg_printf("\nEvent type is %s\n", names[e->type - 1]);
    dbg_printf("Associated customer is ");
    if(e->customer_var == NULL) dbg_printf("NULL\n");
    else dbg_printf("%d\n", e->customer_var->customer_index);
    dbg_printf("Associated teller is ");
    if(e->teller_var == NULL) dbg_printf("NULL\n");
    else dbg_printf("%d\n", e->teller_var->teller_number);
    dbg_printf("Time of event is %lf\n", e->time);
}

void customer_arrival(event* e){
    
    func_pointer_use++;

    int teller_choice = 0;

    if(mode == 0){
        int min = INF;
        for(int i = 0; i < num_tellers; i++){
            int l = t[i]->state + customer_queue_length(t[i]->customer_queue_var);
            if(l < min) min = l;
        }

        int tot_min = 0;
        for(int i = 0; i < num_tellers; i++){
            if(min == t[i]->state + customer_queue_length(t[i]->customer_queue_var)){
                tot_min++;
            }
        }
    
        int choice = rand() % tot_min;
        int idx = 0;
    
        for(int i = 0; i < num_tellers; i++){
            if(min == t[i]->state + customer_queue_length(t[i]->customer_queue_var)){
                if(idx == choice){
                    teller_choice = i;
                    break;
                }
                idx++;
            }
        }
    }
    else{
        teller_choice = random_teller[cur_teller];
        if(cur_teller < num_tellers - 1) cur_teller++;
    }

    customer* c = e->customer_var;
    c->teller_number = teller_choice;

    c->service_time = 2 * avg_service_time * rand()/((double) RAND_MAX);//t[teller_choice]->service_time;  
    
    double service_starting_time; 
    if(t[teller_choice]->customer_queue_var->front != NULL){
        service_starting_time = 
            t[teller_choice]->customer_queue_var->back->customer_var->service_timestamp + t[teller_choice]->customer_queue_var->back->customer_var->service_time;
        //dbg_printf("Queue is not empty, adding the value to get the service starting time as %f\n", service_starting_time);
    }
    else{
        if(!used[t[teller_choice]->teller_number]){
            service_starting_time = time_now;
            //dbg_printf("This is being used for the first time, so we can start right away at %f\n", service_starting_time);
        }
        else{ //sleeping or working
            service_starting_time = wake_times[t[teller_choice]->teller_number];
            //dbg_printf("The teller is busy, so service starts when the teller wakes up at %f\n", service_starting_time);
        }
    }

    c->service_timestamp = service_starting_time;

    event* start = new_event();
    start->type = SERVICE;
    start->customer_var = c;
    start->teller_var = t[teller_choice];
    start->time = service_starting_time - EPS;
    start->action = customer_service;

    insert_event_ordered(q_e, start);

    customer_push(t[teller_choice]->customer_queue_var, c);
    
    return;
}

void customer_service(event* e){
    func_pointer_use++;

    teller* te = e->teller_var;
    
    used[te->teller_number] = 1;
    te->state = 1;

    assert(te->customer_queue_var->front != NULL);
    customer* c_ = te->customer_queue_var->front->customer_var;
    customer_pop(te->customer_queue_var);
    customer* c = e->customer_var;

    assert(c_ == c);
    
    wait_time_till_teller[c->customer_index] = abs(time_now - c->arrival_time + EPS);

    wake_times[te->teller_number] = time_now + c->service_time;

    event* end = new_event();
    end->type = DEPARTURE;
    end->customer_var = c;
    end->teller_var = te;
    end->time = time_now + c->service_time - 2*EPS;
    end->action = customer_departure;

    insert_event_ordered(q_e, end);

    return;
}

void customer_departure(event* e){
    
    time_spent_in_bank[e->customer_var->customer_index] = time_now - e->customer_var->arrival_time;
    total_time_for_all_customers = time_now;
    customers_processed++;
    teller_service_time[e->teller_var->teller_number] += e->customer_var->service_time;
    teller_wake_up(e);
    free(e->customer_var);
    return;
}

void teller_wake_up(event* e){
    func_pointer_use++;
    //do the same thing as above, except don't delete the customer variable, and don't change the number of customers_processed
    teller* te = e->teller_var;
    if(customer_queue_length(te->customer_queue_var) == 0){
        int nonempty = 0;
        for(int i = 0; i < num_tellers; i++){
            if(customer_queue_length(t[i]->customer_queue_var)){
                nonempty++;
            }
        }
        if(nonempty == 0){
            
            event* wake = new_event();
            
            te->state = 0;

            wake->type = WAKE;
            wake->customer_var = NULL;
            wake->teller_var = te;
            wake->time = time_now + MAX_IDLE_TIME * (rand()/((double) RAND_MAX));
            wake->action = teller_wake_up;

            wake_times[te->teller_number] = wake->time;

            insert_event_ordered(q_e, wake);

        }
        else{
            int teller_choice = rand() % nonempty;
            int teller_number = -1;
            int done = 0;
            for(int i = 0; i < num_tellers; i++){
                if(customer_queue_length(t[i]->customer_queue_var)){
                    if(done == teller_choice){
                        teller_number = i;
                        break;
                    }
                    done++;
                }
            }
            customer* customer_to_be_served = t[teller_number]->customer_queue_var->front->customer_var;
            
            customer_push(te->customer_queue_var, customer_to_be_served);
            
            customer_node* nc = t[teller_number]->customer_queue_var->front;
            event_node* en = q_e->front;
            event_node* prev = NULL;
            while(en != NULL && nc != NULL){
                if(en->e->customer_var == nc->customer_var){
                    if(prev == NULL){ 
                        q_e->front = en->next;
                        if(q_e->front == NULL){
                            q_e->back = NULL;
                        }
                        free(en);
                        en = q_e->front;
                    }
                    else if(en->next == NULL){ 
                        q_e->back = prev;
                        if(q_e->back == NULL){
                            q_e->front = NULL;
                        }
                        free(en);
                        en = NULL;
                        prev->next = en;
                    }
                    else{ 
                        prev->next = en->next;
                        free(en);
                        en = prev->next;
                    }
                    nc = nc->next;
                }
                else{
                    prev = en;
                    en = en->next;
                }
            }
            
            customer_pop(t[teller_number]->customer_queue_var);

            customer_to_be_served->teller_number = te->teller_number;
            customer_to_be_served->service_timestamp = time_now + EPS; 
            event* service = new_event();
            service->type = SERVICE;
            service->customer_var = customer_to_be_served;
            service->teller_var = te;
            service->time = customer_to_be_served->service_timestamp;
            service->action = customer_service;

            insert_event_ordered(q_e, service);

            nc = t[teller_number]->customer_queue_var->front;
            while(nc != NULL){
                event* e = new_event();
                e->type = SERVICE;
                e->customer_var = nc->customer_var;
                e->teller_var = t[teller_number];
                e->time = nc->customer_var->service_timestamp - customer_to_be_served->service_time;
                e->action = customer_service;

                insert_event_ordered(q_e, e);

                nc = nc->next;
            }
        }
    }

    return;
}

double min(double a, double b){
    if(a < b) return a; else return b;
}

void run_simulation(char* argv[], int option, int nt){

    time_now = 0.0;

    num_customers = atoi(argv[1]);
    if(option) num_tellers = nt; else num_tellers = atoi(argv[2]);
    
    sim_time = MINUTES_TO_SEC * atof(argv[3]);
    avg_service_time = MINUTES_TO_SEC * atof(argv[4]);

    customers_processed = 0;
    func_pointer_use = 0;
    cur_teller = 0;

    total_time_for_all_customers = 0.0;
    time_spent_in_bank = (double*) malloc(num_customers * sizeof(double));
    wait_time_till_teller = (double*) malloc(num_customers * sizeof(double));
    teller_service_time = (double*) malloc(num_tellers * sizeof(double));
    arrival_times = (double*) malloc(num_customers * sizeof(double));
    random_teller = (int*) malloc(num_tellers * sizeof(double));
    if((start == 0 && option == 0) || option == 1){
        raw_arrival_times = (double*) malloc(num_customers * sizeof(double));
        teller_service_times = (double*) malloc(num_tellers * sizeof(double));
        for(int i = 0; i < num_customers; i++){
            raw_arrival_times[i] = sim_time * rand() / ((double) RAND_MAX);
        }
        for(int i = 0; i < num_tellers; i++){
            teller_service_times[i] = 2 * avg_service_time * rand() / ((double) RAND_MAX);
        }
    }

    q_e = new_event_queue();
    t = (teller**) malloc(num_tellers * sizeof(teller*));
    used = (int*) malloc(num_tellers * sizeof(int));
    wake_times = (double*) malloc(num_tellers * sizeof(double));

    for(int i = 0; i < num_customers; i++){
        time_spent_in_bank[i] = -1.0;
        wait_time_till_teller[i] = -1.0;
    }

    //change used[i] to 0 if we need to revert back
    for(int i = 0; i < num_tellers; i++){
        used[i] = 0;
        wake_times[i] = 0.0;
        teller_service_time[i] = 0.0;
        t[i] = new_teller();
        t[i]->teller_number = i;
        t[i]->customer_queue_var = new_customer_queue();
        t[i]->service_time = teller_service_times[i];
    }

    //comment this out if need to revert back
    /*for(int i = 0; i < num_tellers; i++){

        teller* te = t[i];

        event* wake = new_event();
 
        te->state = 0;
 
        wake->type = WAKE;
        wake->customer_var = NULL;
        wake->teller_var = te;
        wake->time = time_now + MAX_IDLE_TIME * (rand()/((double) RAND_MAX));
        wake->action = teller_wake_up;
        wake_times[te->teller_number] = wake->time;
 
        insert_event_ordered(q_e, wake);

    }*/

    for(int i = 0; i < num_tellers; i++) random_teller[i] = i;
    for(int i = num_tellers - 1; i >= 0; i--){
        int index = rand() % (i + 1);
        int temp = random_teller[index];
        random_teller[index] = random_teller[i];
        random_teller[i] = temp;
    }

    for(int i = 0; i < num_customers; i++){

        customer* c = new_customer();
        c->arrival_time = raw_arrival_times[i];
        c->customer_index = i;
        
        event* e = new_event();
        e->type = ARRIVAL;
        e->customer_var = c;
        e->teller_var = NULL;
        e->action = customer_arrival;
        e->time = c->arrival_time;
        func_pointer_use++;
        
        insert_event_ordered(q_e, e);
    }

    int customer_number_ = 0;
    event_node* enc = q_e->front;
    while(enc != NULL){
        enc->e->customer_var->customer_index = customer_number_;
        arrival_times[customer_number_] = enc->e->customer_var->arrival_time;
        customer_number_++;
        enc = enc->next;
    }

    if(!option){
        event_node* en_ = q_e->front;
    
        while(customers_processed < num_customers){//time_now <= sim_time){
            event *e = top(q_e);
            time_now = e->time;
            if(time_now > sim_time) break; 
            //print_event(e);
            pop(q_e);
            e->action(e);
            //print_all();
            free(e);
        }
        
        //redundant
        en_ = q_e->front;
        while(en_ != NULL){
            event *e = en_->e;
            if(e->type == DEPARTURE){
                teller_service_time[e->teller_var->teller_number] += sim_time - e->customer_var->service_timestamp;
            }
            en_ = en_->next;
        }
        if(!option) printf("Number of tellers is %d\n", num_tellers); 
        if(!option) printf("\nTime spent in the bank statistics\n");
        
        //redundant
        for(int i = 0; i < num_customers; i++){
            if(time_spent_in_bank[i] == -1.0){
                time_spent_in_bank[i] = sim_time - arrival_times[i];
            }
        }
        
        /*for(int i = 0; i < num_customers; i++){
            dbg_printf("Time spent by customer number %d is %lf and arrival was at %lf\n", i, time_spent_in_bank[i], arrival_times[i]);
        }*/
    
        double mean = 0.0;
        double stddev = 0.0;
        for(int i = 0; i < num_customers; i++){
            mean += time_spent_in_bank[i];
            stddev += time_spent_in_bank[i] * time_spent_in_bank[i];
        }
        mean /= num_customers;
        stddev /= num_customers;
        stddev -= mean * mean;
        stddev = sqrt(stddev);
    
        if(!option) printf("Mean of the time spent by customers is %lf and the standard deviation is %lf\n", mean, stddev);
    
        if(!option) printf("\nWait time till seen by a teller statistics\n");
        
        /*for(int i = 0; i < num_customers; i++){
            wait_time_till_teller[i] = min(sim_time - arrival_times[i], wait_time_till_teller[i]);
        }*/
        
        double mx = 0.0;
        for(int i = 0; i < num_customers; i++){
            if(wait_time_till_teller[i] != -1.0){
                if(wait_time_till_teller[i] > mx){
                    mx = wait_time_till_teller[i];
                }
            }
        }
    
        if(!option) printf("\nMaximum wait time of any customer before they were attended to by a teller is %lf\n", mx); 
    
        if(!option) printf("\nTeller service time statistics\n");
        
        /*for(int i = 0; i < num_tellers; i++){
            if(teller_service_time[i] > sim_time) teller_service_time[i] = sim_time;
        }*/
    
        for(int i = 0; i < num_tellers; i++){
            if(!option) printf("Time for which teller number %d was in service was %lf\n", i, teller_service_time[i]); 
        }
    
        if(!option) printf("\nTeller wait time statistics\n");
        for(int i = 0; i < num_tellers; i++){
            if(!option) printf("Time for which teller number %d was idle was %lf\n", i, time_now - teller_service_time[i]);
        }
    
        //Now we need to start working with events which are tagged departure here, and to the corresponding teller's service time, we need to add sim_time - te->customer_var-  >service_timestamp
        //redundant
        en_ = q_e->front;
        while(en_ != NULL){
            event *e = en_->e;
            if(e->type == DEPARTURE){
                teller_service_time[e->teller_var->teller_number] -= sim_time - e->customer_var->service_timestamp;
            }
            en_ = en_->next;
        }
    }
    //this is needed only to find total time taken to serve all the customers
    while(customers_processed < num_customers){
        event *e = top(q_e);
        time_now = e->time;
        pop(q_e);
        e->action(e);
        free(e);
    }
    
    double customer_time_mean = 0.0;
    for(int i = 0; i < num_customers; i++){
        customer_time_mean += time_spent_in_bank[i];
    }
    customer_time_mean /= num_customers;
    if(option) fprintf(datafile, "%d %lf\n", num_tellers, customer_time_mean);

    if(!option) printf("Total time taken by all the customers had the simulation been run till completion is %lf\n", total_time_for_all_customers);

    if(!option) printf("Function pointer was used %d times\n", func_pointer_use);

    for(int i = 0; i < num_tellers; i++){
        free_customer_queue(t[i]->customer_queue_var);
        free(t[i]->customer_queue_var);
        free(t[i]);
    }

    free(t);
    free(used);
    free(wake_times);
    free_event_queue(q_e);
    free(q_e);
    free(time_spent_in_bank);
    free(wait_time_till_teller);
    free(teller_service_time);
    free(arrival_times);
    free(random_teller);

    if((start && option == 0) || option == 1){
        free(raw_arrival_times);
        free(teller_service_times);
    }

}

int main(int argc, char* argv[]){
    
    srand(time(NULL));
    
    printf("******************** Simulation starting for multi-queue system ********************\n");

    mode = 0; //multi queue
    start = 0;
    run_simulation(argv, 0, 0);

    printf("\n\n******************** Simulation starting for common-queue system ********************\n");

    mode = 1;//single queue
    start = 1;
    run_simulation(argv, 0, 0);

    gnuplotpipe = popen("gnuplot -persistent", "w");
    datafile = fopen("output/data.dat", "w");
    //now we start doing the simulations with queues with different numbers of tellers
    for(int i = 2; i < 50; i += 2){
        if(argc == 6){
            mode = 0;
            run_simulation(argv, 1, i);
        }
        mode = 1;
        run_simulation(argv, 1, i);
    }
    fclose(datafile);
    if(argc == 6){
        fprintf(gnuplotpipe, "set term png; set output 'output/plot.png'; plot 'output/data.dat' every 2::0 using 1:2 title \"Multiple queue\", 'output/data.dat' every 2::1 using 1:2 title\"Single queue\";\n");
    }
    else{
        fprintf(gnuplotpipe, "set term png; set output 'output/plot.png'; plot 'output/data.dat' using 1:2 title \"Single queue\";\n");
    }
    fclose(gnuplotpipe);
}
