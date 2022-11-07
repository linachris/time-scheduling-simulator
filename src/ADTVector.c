///////////////////////////////////////////////////////////
//
// ADT Vector Implementation Using Dynamic Array.
// insert/remove only at/from the end of the vector
//
///////////////////////////////////////////////////////////

#include <stdlib.h>
#include <assert.h>

#include "ADTVector.h"

// The initial allocated size
#define VECTOR_MIN_CAPACITY 10
#define GROWTH_FACTOR 2		// for complexity's sake

struct vector_node {
	void* data;
};

struct vector {
	VectorNode* array;			// Our data, array of struct vector_node
	int capacity;				// Total allocated memory(when full, we increase it according to the growth factor "a")
	int size;					// Number of inserted elements
	DestroyFunc destroy_func;
};

// The vector is implemented using an array of VectorNodes and on every insert the array's capacity > GROWTH_FACTOR*size,
// or else we reallocate space and copy all the elements of the array
Vector* vector_create(int size, DestroyFunc destroy_value) {
	Vector* vec = malloc(sizeof(*vec));
	vec->size = size;
	vec->destroy_func = destroy_value;

	// Allocating space for the array. The vector has size not-initialized elements, but we allocate space for
	// at least VECTOR_MIN_CAPACITY, to avoid too many array's resizes
	vec->capacity = size < VECTOR_MIN_CAPACITY ? VECTOR_MIN_CAPACITY : size;
	vec->array = calloc(vec->capacity, sizeof(*vec->array));
	
	return vec;
}

int vector_size(Vector* vec) { return vec->size; }

void* vector_get_at(Vector* vec, int pos) { 
	assert(pos >= 0 && pos < vec->size);	// pos in [0, vec->size-1]

	return vec->array[pos].data;
}

void vector_set_at(Vector* vec, int pos, void* value) {
	assert(pos >= 0 && pos < vec->size);	// pos in [0, vec->size-1]
	
	// replacing the element and freeing the memory from the old one if it's != NULL
	if ((value != vec->array[pos].data) && (vec->destroy_func != NULL))
		vec->destroy_func(vec->array[pos].data);
	
	vec->array[pos].data = value;
}

// We allocate more memory than needed, so that insert is O(1) amortized,
// since there'll be free space, and reallocation + elements' copying takes O(n) time.
// vec->array is being resized 
void vector_insert_last(Vector* vec, void* value) {
	
	// If the array's size has reached the maximum capacity, we resize it according to the growth_factor so that
	// there's free space for future inserts, and the array is not resized each time (time-consuming)
	if (vec->capacity == GROWTH_FACTOR*vec->size) {
		vec->capacity *= GROWTH_FACTOR;
		vec->array = realloc(vec->array, vec->capacity*sizeof(*vec->array));	// realloc frees the old pointer
	}
	// adding the new element in the array and updating vec's size
	vec->array[vec->size].data = value;
	vec->size++;
}

void vector_remove_last(Vector* vec) {
	assert(vec->size);

	if (vec->destroy_func != NULL)
		vec->destroy_func(vec->array[vec->size-1].data);
	
	vec->size--;
	
	// If only 1/(growth_factor*2) of the array's capacity is full, we resize the array to 1/growth_factor of its capacity
	// so that we don't have too much memory waste, and there's still space for insert and remove.
	if ((vec->capacity > vec->size * 2 * GROWTH_FACTOR) && (vec->capacity > VECTOR_MIN_CAPACITY * GROWTH_FACTOR)) {
			vec->capacity /= GROWTH_FACTOR;
			vec->array = realloc(vec->array, vec->capacity * sizeof(*vec->array));
	}
}

void* vector_find(Vector* vec, void* value, CompareFunc compare) {
	VectorNode* v_find = vector_find_node(vec, value, compare);
	return v_find == NULL ? NULL : v_find->data;
}

DestroyFunc vector_set_destroy_value(Vector* vec, DestroyFunc destroy_value) {
	DestroyFunc old = vec->destroy_func;
	vec->destroy_func = destroy_value;
	return old;
}

void vector_destroy(Vector* vec) {
	// calling the destroy_func if it's != NULL for every element of the array
	if (vec->destroy_func != NULL)
		for (int i = 0; i < vec->size; ++i)			// the |actual elements| = vec's size
			vec->destroy_func(vec->array[i].data);

	// freeing the array and the struct as well
	free(vec->array);
	free(vec);
}

// =================== Vector Iteration =================== //

VectorNode* vector_first(Vector* vec) { return vec->size == 0 ? NULL : &vec->array[0]; }

VectorNode* vector_last(Vector* vec) { return vec->size == 0 ? NULL : &vec->array[vec->size-1]; }

VectorNode* vector_next(Vector* vec, VectorNode* node) { return node == &vec->array[vec->size-1] ? NULL : node + 1; }

VectorNode* vector_previous(Vector* vec, VectorNode* node) { return node == &vec->array[0] ? NULL : node - 1; }

void* vector_node_value(VectorNode* node) { return node->data; }

VectorNode* vector_find_node(Vector* vec, void* value, CompareFunc compare) { 
	for (int i = 0; i < vec->size; ++i)
		if (compare(vec->array[i].data, value) == 0)
			return &vec->array[i];		// found
	
	return NULL;	// not found
}