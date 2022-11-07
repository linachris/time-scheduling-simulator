///////////////////////////////////////////////////////////////////
// ADT Vector
// Abstract dynamic "array"
///////////////////////////////////////////////////////////////////

#pragma once // #include once

#include "common_types.h"

#define VECTOR_FAIL	(Vector*)0

// The vector is implemented using a struct Vector
typedef struct vector Vector;

// Creates and returns a vector with the given size, with all elements initialized to NULL.
// If destroy_value != NULL, then destroy_value(value) is called every time an element is removed or implemented.
Vector* vector_create(int size, DestroyFunc destroy_value);

// Size of vec
int vector_size(Vector* vec);

// Inserts element value at the end of the vec, size incremented by 1
void vector_insert_last(Vector* vec, void* value);

// Removes the last element of the vec, size decremented by 1
void vector_remove_last(Vector* vec);

// Returns the value of the element in the position pos. pos = [0..size-1]
void* vector_get_at(Vector* vec, int pos);

// Sets the value of the element in the position pos. pos = [0..size-1]
void vector_set_at(Vector* vec, int pos, void* value);

// Finds and returns the first element of the vector with that value, according to the compare function given
// Returns NULL if no element is found with that value
void* vector_find(Vector* vec, void* value, CompareFunc compare);

// Changes the function called at every removal or replacement of an element to destroy_value
// and returns the old destroy_function value
DestroyFunc vector_set_destroy_value(Vector* vec, DestroyFunc destroy_value);

// Frees the memory allocated by the vector
void vector_destroy(Vector* vec);

// =================== Vector Iteration =================== //

// Used to implement every node of the vector
typedef struct vector_node VectorNode;

// Returns first/last node of the vector, or NULL if it's empty
VectorNode* vector_first(Vector* vec);
VectorNode* vector_last(Vector* vec);

// Returns the next/previous node of the given node, or NULL if the node doesn't have one
VectorNode* vector_next(Vector* vec, VectorNode* node);
VectorNode* vector_previous(Vector* vec, VectorNode* node);

// Returns the value of the given node
void* vector_node_value(VectorNode* node);

// Finds and returns the first node of the vector with an element of that value, according to the compare function given
// Returns NULL if no element is found with that value
VectorNode* vector_find_node(Vector* vec, void* value, CompareFunc compare);
