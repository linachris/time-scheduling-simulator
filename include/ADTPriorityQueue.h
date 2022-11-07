///////////////////////////////////////////////////////////////////
// ADT Priority Queue
///////////////////////////////////////////////////////////////////

#pragma once

#include "common_types.h"
#include "ADTVector.h"

// PQ is implemented using a stuct PQ
typedef struct priority_queue PriorityQueue;
typedef struct priority_queue_node PriorityQueueNode;

// Creates and returns a PQ, with the elements' order being according to the compare function given
// If destroy_value != NULL then destroy_value(value) is called everytime an element is removed
// If values != NULL, the PQ is initialized with the elements of the Vector values
PriorityQueue* pqueue_create(CompareFunc compare, DestroyFunc destroy_value, Vector* values);

// PQ size
int pqueue_size(PriorityQueue* pqueue);

// PQ maximum element (according to compare function given)
void* pqueue_max(PriorityQueue* pqueue);

// Element with the given value is inserted to the PQ
PriorityQueueNode* pqueue_insert(PriorityQueue* pqueue, void* value);

// Removes the maximum(according to compare function given) element of the PQ and returns it
void* pqueue_remove_max(PriorityQueue* pqueue);

// Changes the function called at every removal or replacement of an element to destroy_value
// and returns the old destroy_function value
DestroyFunc pqueue_set_destroy_value(PriorityQueue* pqueue, DestroyFunc destroy_value);

// Deallocates the memory used by pqueue
void pqueue_destroy(PriorityQueue* pqueue);

// Returns the value of the node
void* pqueue_node_value(PriorityQueueNode* node);

// Removes the node, which can be in any position of the pqueue
// !! It's mandatory that we call pqueue_update_order() after
void pqueue_remove_node(PriorityQueue* pqueue, PriorityQueueNode* node);

// Updates the pqueue, after a change in the order of the pqueue because of the removal of node.
void pqueue_update_order(PriorityQueue* pqueue, PriorityQueueNode* node);

// function for handling the processes in the pq
Vector* pqueue_get_vector(PriorityQueue* pq);

// Returns the value of the node that is in the position node_id in the pqueue
void* node_value(PriorityQueue* pqueue, int node_id);