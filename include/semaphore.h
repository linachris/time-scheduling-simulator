#include <stdbool.h>

// a semaphore is a pointer to this struct
typedef struct semaphore* Semaphore;

// returns an array of S pointers to struct semaphore
Semaphore* create_semaphores(int S);

// deallocated the memory of the semaphores created
void destroy_semaphores(Semaphore* sem_set, int S);

// process enters its CS
void sem_down(Semaphore sem, int pid);

// process exits its CS
void sem_up(Semaphore sem);

// returns the pid of the proccess using the semaphore now
int sem_used_by_process(Semaphore sem);