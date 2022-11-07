#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "../include/semaphore.h"
#include "common_types.h"
#include "ADTPriorityQueue.h"
#include "ADTVector.h"

//// ======================================================== P R O C E S S ======================================================== ////
typedef struct process {
	int pid;
	int priority;
	double arrival_time;
	double lifetime;
	int time_slots_running;
	int start_time;
	int end_time;
	int waiting_time;
	int blocked_time;

	double cs_time;
	int cs_enter_probability;
	int cs_time_executed;
	Semaphore sem_alloc;
} Process;

// compare based first on arrival time, then on priority, and then on pid
int process_pool_compare(void *a, void *b)
{
    int to_return = (((Process*)b)->arrival_time - ((Process*)a)->arrival_time);
	if (to_return == 0)
		return (((Process*)b)->priority - ((Process*)a)->priority);
	if (to_return == 0)
		return (((Process*)b)->pid - ((Process*)a)->pid);
    return to_return;
}

// compare based first on priority, then on arrival time, and then on pid
int ready_pq_compare(void *a, void *b)
{
    int to_return = (((Process*)b)->priority - ((Process*)a)->priority);
	if (to_return == 0)
		return (((Process*)b)->arrival_time - ((Process*)a)->arrival_time);
	if (to_return == 0)
		return (((Process*)b)->pid - ((Process*)a)->pid);
    return to_return;
}

// compare based first on end_time, then on arrival time, and then on pid
int finished_pq_compare(void *a, void *b) {
	int to_return = (((Process*)b)->end_time - ((Process*)a)->end_time);
	if (to_return == 0)
		return (((Process*)b)->arrival_time - ((Process*)a)->arrival_time);
	if (to_return == 0)
		return (((Process*)b)->pid - ((Process*)a)->pid);
    return to_return;
}

double rand_exponential(double lambda) { return -log(1.0 - rand() / (RAND_MAX + 1.0))/lambda; }

int rand_uniform(int low, int high) {
	int range = high - low +1;
	double rand_var = rand() / (RAND_MAX + 1.0);
	return (rand_var*range) + low;
}

// creates and initializes total_processes Processes and returns a PQ of them
PriorityQueue* processes_generator(int total_processes, double lambda_arrival, double lambda_lifetime, double lambda_cs_time) {
	
	// create process pool, ordered by arrival_time(process_pool_compare function)
	PriorityQueue* processes_pq = pqueue_create(process_pool_compare, NULL, NULL);
	double time = 0;
	
	for (int i = 0; i < total_processes; i++) {
		Process* proc = malloc(sizeof(*proc));
		
		proc->pid = i;
		proc->priority = rand_uniform(1, 7);
		
		// the arrival time of the current process = arrival_time of the previously created process("time" in our code)
		// + the exponential time between 2 arrivals
		proc->arrival_time = time + rand_exponential(lambda_arrival);
		time = proc->arrival_time;

		proc->lifetime = rand_exponential(lambda_lifetime);
		proc->lifetime += proc->arrival_time;	// lifetime counts from the moment the process arrives

		proc->time_slots_running = 0;
		proc->start_time = 0;
		proc->end_time = 0;
		proc->waiting_time = 0;
		proc->blocked_time = 0;

		proc->cs_time = rand_exponential(lambda_cs_time);
		proc->cs_time_executed = 0;
		proc->sem_alloc = NULL;

		// initialization is complete so insert it into the pqueue
		pqueue_insert(processes_pq, proc);
	}
	return processes_pq;
}

// Function for processes ~~ waiting ~~ in a pqueue to be executed
void incr_proc_waiting_time(PriorityQueue* pq, int* waiting_time_slots) {
	Vector* vec = pqueue_get_vector(pq);

	// incrementing the processes' waiting_time
	for (int i = 0; i < vector_size(vec); i++) {
		// ((Process*)pqueue_node_value(node_value(pq, i+1)))->waiting_time++;
		Process* p_to_incr = pqueue_node_value(node_value(pq, i+1));
		p_to_incr->waiting_time++;
		waiting_time_slots[p_to_incr->priority - 1]++;
	}
}

// checking if any process is not alive any more, except for the one that is already running (that's a seperate check)
void checkIfAnyProcessPassedItsLifetime(PriorityQueue* ready_pq, PriorityQueue* finished_pq, int current_time, Process* curr_proc_running) {
	Vector* vec = pqueue_get_vector(ready_pq);

	for (int i = 0; i < vector_size(vec); i++) {
		Process* prob_fin_proc = pqueue_node_value(node_value(ready_pq, i+1));		// probably_finished_process

		if (prob_fin_proc->lifetime <= current_time) {
			pqueue_remove_node(ready_pq, (PriorityQueueNode*)node_value(ready_pq, i + 1));
			prob_fin_proc->end_time = current_time;
			
			// if that process is in its CS, force up()
			if (prob_fin_proc->sem_alloc != NULL) {
				// running its CS rn
				if (sem_used_by_process(prob_fin_proc->sem_alloc) == prob_fin_proc->pid)
					sem_up(prob_fin_proc->sem_alloc);
				
				prob_fin_proc->sem_alloc = NULL;
			}
			pqueue_insert(finished_pq, prob_fin_proc);	// it's finished

			// the prob_fin_proc is the curr_proc_running
			if ((curr_proc_running != NULL) && (prob_fin_proc->pid == curr_proc_running->pid))	// if the lifetime of the process currently running just ended, we change the value 
				curr_proc_running = NULL;	// of the currently running to NULL, so in the next time_slot, we don't compare priorities again with other processes
		}
		vec = pqueue_get_vector(ready_pq);
	}
}

// deallocating memory 
void free_resources(PriorityQueue* finished_pqueue, PriorityQueue* ready_pqueue, PriorityQueue* processes_pool, Semaphore* sem_set, int S) {
	pqueue_destroy(finished_pqueue); // all the processes, that we want to deallocate memory for are in the finished pqueue, in the end
	pqueue_destroy(ready_pqueue);
	pqueue_destroy(processes_pool);
	destroy_semaphores(sem_set, S);
}
//// ========================================================  S I M U L A T O R  ======================================================== ////

int main(int argc, char* argv[]) {

	srand(time(NULL));

	double lambda_arrival, lambda_lifetime, lambda_cs_time;
	int total_processes, k, S;
	int running_time_slots[7]; // time slots for each of the 7 sets of priority processes..priority-1based : running_time_slots[0]..priority-2based : running_time_slots[1], etc
	int waiting_time_slots[7];
	int blocked_time_slots[7];
	int cs_time_slots[7];
	int curr_time = 0;
	FILE* running_state_fp;
	Process* curr_proc_running = NULL;
	Semaphore* sem_set;
	PriorityQueue* processes_pool, *ready_pqueue, *finished_pqueue;

	// Correct number of arguments needed
	if (argc != 7) {
		fprintf(stderr, "Error! Correct Usage: ./simulator <lambda_arrival> <lambda_lifetime> <lambda_cs_time> <total_processes> <k: down() probability> <S: Num of Semaphores>\n");
		exit(EXIT_FAILURE);
	}
	
	lambda_arrival = atof(argv[1]);
	lambda_lifetime = atof(argv[2]);
	lambda_cs_time = atof(argv[3]);
	total_processes = atoi(argv[4]);
	k = atoi(argv[5]);
	S = atoi(argv[6]);

	// Initializing the file of the 'running state' and deleting its contents if it already exists
	fclose(fopen("running_state.log", "w"));

	// initialization
	for (int i = 0; i < 7; i++) {
		running_time_slots[i] = 0;
		waiting_time_slots[i] = 0;
		blocked_time_slots[i] = 0;
		cs_time_slots[i] = 0;
	}

	sem_set = create_semaphores(S);
	processes_pool = processes_generator(total_processes, lambda_arrival, lambda_lifetime, lambda_cs_time);	// all created processes
	ready_pqueue = pqueue_create(ready_pq_compare, NULL, NULL);	// all processes that have arrived
	finished_pqueue = pqueue_create(finished_pq_compare, free, NULL);	// all processes that are finished, each node holds a Process* for which we allocated memory before, so free it upon destroy.

	// while there are still processes created and not all done yet
	// a time slot is this while loop
	while (pqueue_size(finished_pqueue) != total_processes) {
		Process* proc_insert, *competitor_proc;

		// obtains the first arrived processes and inserts them into the ready_pqueue
		while((pqueue_size(processes_pool) != 0) && (proc_insert = pqueue_max(processes_pool)) && (proc_insert->arrival_time <= curr_time)) {
			Process* ready_process = pqueue_remove_max(processes_pool);
			pqueue_insert(ready_pqueue, ready_process);
		}

		// the current process is not alive any more
		if ((curr_proc_running != NULL) && (curr_proc_running->lifetime <= curr_time)) {
				curr_proc_running->end_time = curr_time;

			// if the process is at its CS, force up()
			if (curr_proc_running->sem_alloc != NULL) {
				// running its CS rn
				if (sem_used_by_process(curr_proc_running->sem_alloc) == curr_proc_running->pid)
					sem_up(curr_proc_running->sem_alloc);
					
				curr_proc_running->sem_alloc = NULL;
			}

			// printing the running state of the process to an external file
			running_state_fp = fopen("running_state.log", "a");
			if (running_state_fp == NULL) {
				// deallocating memory 
				free_resources(finished_pqueue, ready_pqueue, processes_pool, sem_set, S);
				error_exit("running_state_fp: fopen failed");
			}
			fprintf(running_state_fp, "Finishing now Process with PID: %d\n", curr_proc_running->pid);
			fclose(running_state_fp);
			
			pqueue_insert(finished_pqueue, curr_proc_running);
			curr_proc_running = NULL;
		}

		// before extracting the max_process from ready_pq:
		// checks for non alive processes in the ready_pqueue, where they are all supposed to be alive
		// and if there exist, it takes them from the ready_pq to the finished_pq, every time slot passing by
		checkIfAnyProcessPassedItsLifetime(ready_pqueue, finished_pqueue, curr_time, curr_proc_running);

		// =========================================================================================================================================== //

		// There is another process running, so we have to obtain the process with the highest priority
		// from the ready_pqueue, and compare it with the one currently running. If it's higher, it'll take
		// the curr_process's place(only if its not in the CS, else it'll be blocked) which will be inserted back into the ready_pqueue.
		if ((pqueue_size(ready_pqueue) != 0) && (curr_proc_running != NULL)) {
			competitor_proc = pqueue_max(ready_pqueue);
			competitor_proc->cs_enter_probability = rand() % 101;

			// The curr_proc_running has attempted to enter its CS, and it's either running or blocked
			if (curr_proc_running->sem_alloc != NULL) {
				// Running in CS, so the curr_proc_running is gonna continue to run in its CS
				if (sem_used_by_process(curr_proc_running->sem_alloc) == curr_proc_running->pid) {
					
					// the competitor process attempts to enter its CS and is blocked, since the curr process is in its CS
					if (competitor_proc->cs_enter_probability >= k) {
						competitor_proc->blocked_time++;
						blocked_time_slots[competitor_proc->priority - 1]++;
					}
				}
				// It was blocked. The highest priority process is gonna run
				else {
					// We obtain the highest priority process, which will be stored as curr_proc_running
					if (competitor_proc->priority < curr_proc_running->priority) {  // the competitor_proc has higher priority and must take its place!

						// The competitor is gonna run, so the curr_proc_running is blocked
						curr_proc_running->cs_enter_probability = rand() % 101;
						if (curr_proc_running->cs_enter_probability >= k) {
							curr_proc_running->blocked_time++;
							blocked_time_slots[curr_proc_running->priority - 1]++;
						}
						pqueue_remove_max(ready_pqueue);							// removing competitor_process from the ready_pq, since it is gonna run
						pqueue_insert(ready_pqueue, curr_proc_running);  			// the previously curr_process_running is pushed back into the ready_queue
					
						curr_proc_running = competitor_proc;						// and the competitor is the new current process running
						if(curr_proc_running->start_time == 0)						// if it's the beginning of its execution
							competitor_proc->start_time = curr_time;
					}
					// else the curr_proc_running has a higher priority than the competitor_proc, so it continues running
				}
			}

			// The sem_alloc is NULL, so the curr_proc_running has never attempted to enter its CS, or previous CS was done.
			// So we find the process with the higher priority to run
			else {
				// We obtain the highest priority process, which will be stored as curr_proc_running
				if (competitor_proc->priority < curr_proc_running->priority) {  // the competitor_proc has higher priority and must take its place!
					
					// The competitor is gonna run, so the curr_proc_running is blocked
					curr_proc_running->cs_enter_probability = rand() % 101;
					if (curr_proc_running->cs_enter_probability >= k) {
						curr_proc_running->blocked_time++;
						blocked_time_slots[curr_proc_running->priority - 1]++;
					}
					pqueue_remove_max(ready_pqueue);							// removing competitor_process from the ready_pq, since it is gonna run
					pqueue_insert(ready_pqueue, curr_proc_running);  			// the previously curr_process_running is pushed back into the ready_queue
				
					curr_proc_running = competitor_proc;						// and the competitor is the new current process running
					if(curr_proc_running->start_time == 0)						// if it's the beginning of its execution
						competitor_proc->start_time = curr_time;
				}
				// else the curr_proc_running has a higher priority than the competitor_proc, so it continues running
			}
		}
		// =========================================================================================================================================== //
		// There is no other process running, so none of the semaphores is being used.
		// The last process is going to run here
		if ((pqueue_size(ready_pqueue) != 0) && (curr_proc_running == NULL)) {
			curr_proc_running = pqueue_remove_max(ready_pqueue);	// the highest priority process will be running
			if(curr_proc_running->start_time == 0)
				curr_proc_running->start_time = curr_time;			// it's the beginning of its execution
		}
		// =========================================================================================================================================== //
		// Now, we have the current process running with the highest priority, if it's not NULL, and we're gonna see if it's gonna enter its CS
		if (curr_proc_running != NULL) {

			// current process running not done with its CS yet, or not having entered its CS yet
			if (curr_proc_running->cs_time_executed < curr_proc_running->cs_time) {

				// The process that was blocked before from entering its CS, enters now
				if (curr_proc_running->sem_alloc != NULL) {
					sem_down(curr_proc_running->sem_alloc, curr_proc_running->pid); // the semaphore is avalaible, so the process enters its CS
					curr_proc_running->cs_time_executed++;
					cs_time_slots[curr_proc_running->priority - 1]++;
				}
				// Hasn't attempted sem_down() yet, or previous CS was done, so it enters its CS with a probability
				else {
					// Checking to see if the process is gonna enter its CS, depending on the probability
					curr_proc_running->cs_enter_probability = rand() % 101;
					if (curr_proc_running->cs_enter_probability >= k) {
						curr_proc_running->sem_alloc = sem_set[rand_uniform(1, S) - 1];
						sem_down(curr_proc_running->sem_alloc, curr_proc_running->pid); // the semaphore is avalaible, so the process enters its CS
						
						curr_proc_running->cs_time_executed++;
						cs_time_slots[curr_proc_running->priority - 1]++;
					}
					// else not entering its CS, but not inserting back into the pq, since it can continue to run outside the CS
				}
			}
			// it's "cs_time_executed >= cs_time" so its CS is done..Setting sem_alloc equal to NULL, so that on a possible 
			// next CS enter attempt, it can try to use a different or even the same Semaphore. We don't insert it back into the ready_pq,
			// because it can continue running outside of the CS, till another process with higher priority comes
			else {
				if (sem_used_by_process(curr_proc_running->sem_alloc) == curr_proc_running->pid) {
					sem_up(curr_proc_running->sem_alloc);
				}
				curr_proc_running->cs_time_executed = 0;	// initializing for a possible entry to the CS again
				curr_proc_running->sem_alloc = NULL;
			}

			curr_proc_running->time_slots_running++;
			running_time_slots[curr_proc_running->priority - 1]++; // increment time slot used by this set of processes with this priority
			
			// printing the running state of the process to an external file
			running_state_fp = fopen("running_state.log", "a");
			if (running_state_fp == NULL) {
				// deallocating memory 
				free_resources(finished_pqueue, ready_pqueue, processes_pool, sem_set, S);
				error_exit("running_state_fp: fopen failed");
			}
			fprintf(running_state_fp, "Running now Process with PID: %d, Current Service Time: %d\n", curr_proc_running->pid, curr_proc_running->time_slots_running);
			fclose(running_state_fp);		
		}

		if(pqueue_size(ready_pqueue) != 0)
			incr_proc_waiting_time(ready_pqueue, waiting_time_slots);	// increase waiting time of the functions in the ready_pq, waiting to be executed
		curr_time++;	// next_time_slot
	}

	// Printing waiting, blocked, running, cs state for each set of priorities of the processes
	for (int i = 0; i < 7; i++) {
		printf("Waiting for: %d, Blocked for: %d, Running for: %d, Critical section for: %d time slots for processes with priority: %d\n", waiting_time_slots[i], blocked_time_slots[i], running_time_slots[i], cs_time_slots[i], i + 1);
	}

	// deallocating memory 
	free_resources(finished_pqueue, ready_pqueue, processes_pool, sem_set, S);

	return 0;
}