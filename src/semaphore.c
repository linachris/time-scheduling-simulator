#include <stdlib.h>
#include "../include/semaphore.h"
#include "common_types.h"

struct semaphore {
	int semid;
	// pid of process using this semaphore at the CS.. used_by_pid = -1 if it's not used 
	int used_by_pid;	// else used_by_pid = pid of the process that is currently using it
};

Semaphore* create_semaphores(int S) {
	Semaphore* sem_set = malloc(S*sizeof(*sem_set)); // mem allocation for set of semaphores
	for (int i = 0; i < S; i++) {
		sem_set[i] = malloc(sizeof(*sem_set[i])); 	 // mem allocation for each semaphore
		sem_set[i]->semid = i;						 // and initializing
		sem_set[i]->used_by_pid = -1;				 // not used by any process initially
	}
	return sem_set;
}

void destroy_semaphores(Semaphore* sem_set, int S) {
	for(int i = 0; i < S; ++i)
		free(sem_set[i]);	// deallocating the memory for each semaphore
	free(sem_set); 			// deallocating the memory for the set of semaphore itself
}

void sem_down(Semaphore sem, int pid) { sem->used_by_pid = pid; }

void sem_up(Semaphore sem) { sem->used_by_pid = -1; }

int sem_used_by_process(Semaphore sem) { return sem->used_by_pid; }