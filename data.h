#include "sack_object.h"
#include "individual.h"
#include <pthread.h>

#ifndef DATA_H
#define DATA_H

// structure used to split all problem data between threads
typedef struct data{

	// thread id
	int id;

	// number of threads
	int P;

	// barrier shared by all threads
	pthread_barrier_t *barrier;

    // array with all the objects that can be placed in the sack
	sack_object *objects;

	// number of objects
	int object_count;

	// maximum weight that can be carried in the sack
	int sack_capacity;

	// number of generations
	int generations_count;

	// address of the current_gen array - same array for all threads
	individual **current_generation;

	// address of the next_get array - same array for all threads
	individual **next_generation;

	// array used in merge function - same array for all threads
	individual **merge_generation;

} data;

#endif