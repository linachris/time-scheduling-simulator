#pragma once // #include once
#include <stdbool.h>
#define error_exit(msg)		do { perror(msg); exit(EXIT_FAILURE); \
							} while (false)

// Pointer to a function that compares two elements and returns:
// < 0  if a < b
//   0  if a equal b
// > 0  if a > b
typedef int (*CompareFunc)(void* a, void* b);

// Pointer to function that destroys the element value
typedef void (*DestroyFunc)(void* value);

// compare based first on arrival time, then on priority, and then on pid
int process_pool_compare(void *a, void *b);

// compare based first on priority, then on arrival time, and then on pid
int ready_pq_compare(void *a, void *b);

// compare based first on end_time, then on arrival time, and then on pid
int finished_pq_compare(void *a, void *b);

// exponential distribution
double rand_exponential(double lambda);

// uniform distribution
int rand_uniform(int low, int high);