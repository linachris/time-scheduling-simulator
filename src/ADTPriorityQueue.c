///////////////////////////////////////////////////////////
// ADT PriorityQueue implementation using heap
///////////////////////////////////////////////////////////

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "ADTPriorityQueue.h"
#include "ADTVector.h"

struct priority_queue {
	Vector* vector;				// Vector for the data, so that we have a dynamic array
	CompareFunc compare;		// Order of the values in pqueue
	DestroyFunc destroy_value;	// Function that destroys an element of the vector.
};

// All ids of the nodes are 1-based in the pqueue but 0-based in the vector
struct priority_queue_node {
	void* value;				// Node's value
	int id;						// Position in vector
	PriorityQueue* owner;		// Pointer for accessing the pqueue, from a node
};

void* node_value(PriorityQueue* pqueue, int node_id) {
	// node_id is 1-based, but the vector's nodes are 0-based, so "node_id - 1"
	return vector_get_at(pqueue->vector, node_id - 1);
}

void node_swap(PriorityQueue* pqueue, int node1_id, int node2_id) {
	PriorityQueueNode* node1 = node_value(pqueue, node1_id);
	PriorityQueueNode* node2 = node_value(pqueue, node2_id);
	
	vector_set_at(pqueue->vector, node2_id - 1, node1);
	vector_set_at(pqueue->vector, node1_id - 1, node2);

	// update positions of the nodes in the vector
	int temp = node1->id;
	node1->id = node2->id;
	node2->id = temp;
}

PriorityQueueNode* pqueue_node_create(PriorityQueue* pqueue, void* value, int pos) {
    PriorityQueueNode* node = malloc(sizeof(*node));
    node->value = value;
	node->id = pos;
	node->owner = pqueue;
    return node;
}

// Compare the values of tho nodes, accordinf to the initial compare function
static int compare_pq_nodes(PriorityQueueNode* a, PriorityQueueNode* b) {
	return a->owner->compare(a->value, b->value);
}

// Destroys a PriorityQueueNode, for which the memory is not deallocated yet at the end of the program
static void destroy_pq_node(PriorityQueueNode* node) {
	if (node->owner->destroy_value != NULL)
		node->owner->destroy_value(node->value);
	
	vector_remove_last(node->owner->vector);
	free(node);
}

// Before, all the nodes, except for the node with id node_id that can be greater than its father, satisfy the heap property
// Calling bubble_up restores the heap property
static void bubble_up(PriorityQueue* pqueue, int node_id) {
	// If we've reached the root, we stop
	if (node_id == 1)
		return;
	
	int parent = node_id / 2;

	PriorityQueueNode* parent_node = node_value(pqueue, parent);
	PriorityQueueNode* node = node_value(pqueue, node_id);

	// If the parent has a smaller value than the node, we swap and continue going up recursively
	if (compare_pq_nodes(parent_node, node) < 0) {
		node_swap(pqueue, parent, node_id);
		bubble_up(pqueue, parent);
	}
}

// Before, all the nodes, except for the node with id node_id that can be smaller than one of its children, satisfy the heap property
// Calling bubble_up restores the heap property
static void bubble_down(PriorityQueue* pqueue, int node_id) {
	
	PriorityQueueNode* node = node_value(pqueue, node_id);
	// We find the children of the node
	int left_child = 2 * node_id;
	int right_child = left_child + 1;
	
	// No left children, means no right one
	int size = pqueue_size(pqueue);
	if (left_child > size)
		return;

	// Max of the two children
	int max_child = left_child;
	PriorityQueueNode* left_child_node = node_value(pqueue, left_child);
	
	if (right_child <= size) {
		PriorityQueueNode* right_child_node = node_value(pqueue, right_child);
		if(compare_pq_nodes(left_child_node, right_child_node) < 0)
			max_child = right_child;
	}

	PriorityQueueNode* max_child_node = node_value(pqueue, max_child);

	// If the node is smaller than the max child, we swap and continue going down recursively
	if (compare_pq_nodes(node, max_child_node) < 0) {
		node_swap(pqueue, node_id, max_child);
		bubble_down(pqueue, max_child);
	}
}


static void pqueue_insert_values(PriorityQueue* pqueue, Vector* values) {
	int size = vector_size(values);
	for (int i = 0; i < size; i++) {
		
		// Creating pq_node for inserting into the vector, with an initial position (pqueue_size + 1)
		void* value = vector_get_at(values,i);
		PriorityQueueNode* inserted = pqueue_node_create(pqueue, value, pqueue_size(pqueue) + 1);
		vector_insert_last(pqueue->vector, inserted);
	}
}

// Initializes the heap with the values of the vector values
static void heapify(PriorityQueue* pqueue, Vector* values) {

	// No heap property yet
	pqueue_insert_values(pqueue, values);

	// Visiting all internal nodes in reverse level order
	// and calling bubble_down ,to restore the heap property
	int pq_size = vector_size(pqueue->vector);	
	for (int i = pq_size/2; i > 0; i--)
		bubble_down(pqueue, i);		
}

//// ======================================= ADTPriorityQueue ======================================= ////

PriorityQueue* pqueue_create(CompareFunc compare, DestroyFunc destroy_value, Vector* values) {
	assert(compare != NULL);

	PriorityQueue* pqueue = malloc(sizeof(*pqueue));
	pqueue->compare = compare;
	pqueue->destroy_value = destroy_value;

	// Creating the vector of the values, but not storing the destroy_value too
	// as when we swap 2 elements, destroy_value is gonna be called, which is something we don't want 
	pqueue->vector = vector_create(0, NULL);

	// If values != NULL, we initialize the heap with these values
	if (values != NULL)
		heapify(pqueue, values);

	return pqueue;
}

int pqueue_size(PriorityQueue* pqueue) {
	return vector_size(pqueue->vector);
}

// PQ max is at the root of the heap(id = 1)
void* pqueue_max(PriorityQueue* pqueue) {
	PriorityQueueNode* max = node_value(pqueue, 1);
	return max->value;
}

PriorityQueueNode* pqueue_insert(PriorityQueue* pqueue, void* value) {
	// We add the inserted node at the end of the heap
	PriorityQueueNode* inserted = pqueue_node_create(pqueue, value, pqueue_size(pqueue) + 1);
	vector_insert_last(pqueue->vector, inserted);
	
	// restoring heap property
	// The inserted node can be greater than its parent
	bubble_up(pqueue, inserted->id);

	return inserted;
}

void* pqueue_remove_max(PriorityQueue* pqueue) {
	int last = pqueue_size(pqueue);
	assert(last != 0);

	// destroy the value being removed
	if (pqueue->destroy_value != NULL)
		pqueue->destroy_value(pqueue_max(pqueue));

	// We swap the first with the last node, and we remove the last one
	node_swap(pqueue, 1, last);
	
	PriorityQueueNode* max_node = node_value(pqueue, last);
	void* value_to_return = max_node->value;
	vector_remove_last(pqueue->vector);
	free(max_node); // free the memory allocated in insert

	// The new root can be smaller than one of its children
	// Restoring the heap property
	if (pqueue_size(pqueue) > 1)
		bubble_down(pqueue, 1);
	return value_to_return;
}

DestroyFunc pqueue_set_destroy_value(PriorityQueue* pqueue, DestroyFunc destroy_value) {
	DestroyFunc old = pqueue->destroy_value;
	pqueue->destroy_value = destroy_value;
	return old;
}

void pqueue_destroy(PriorityQueue* pqueue) {
	
	// Removing the PQNodes left in the vector and deallocating the memory
	int vec_size = vector_size(pqueue->vector);
	for (int i = 0; i < vec_size; i++) {
		PriorityQueueNode* to_delete = node_value(pqueue, vector_size(pqueue->vector));
		destroy_pq_node(to_delete);
	}	
	vector_destroy(pqueue->vector);
	free(pqueue);
}
void* pqueue_node_value(PriorityQueueNode* node) {
	return node->value;
}

void pqueue_remove_node(PriorityQueue* pqueue, PriorityQueueNode* node) {
	int last = pqueue_size(pqueue);
	assert(last != 0);

	// Node is the root, so it's the max
	if(node->id == 1) {
		pqueue_remove_max(pqueue);
		return;
	}

	// Destroy the value of the node being removed
	if (pqueue->destroy_value != NULL)
		pqueue->destroy_value(node->value);
	
	// The node can be any node in the heap, so we swap it with the last one and remove the last one
	node_swap(pqueue, node->id, last);
	vector_remove_last(pqueue->vector);
	free(node);

	// The node can be smaller than any of its children, so we restore the heap property
	if (pqueue_size(pqueue) > 1)
		bubble_down(pqueue, 1);
}

void pqueue_update_order(PriorityQueue* pqueue, PriorityQueueNode* node) {

	// Restoring the heap property after the change of order in the heap
	// If the node is greater than its parent, it has to go up
	bubble_up(pqueue, node->id);

	// The node is smaller than its parent, but might be smaller than its children too, so its has to go down
	bubble_down(pqueue, node->id);
}

// function for handling the processes in the pq
Vector* pqueue_get_vector(PriorityQueue* pq) { return pq->vector; }