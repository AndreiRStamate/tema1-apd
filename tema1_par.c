#include <stdlib.h>
#include "data.h"
#include "genetic_algorithm.h"
#include <stdio.h>
#include <pthread.h>

int main(int argc, char *argv[]) {
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	// number of threads
	int P = 0;

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, argc, argv, &P)) {
		return 0;
	}

	pthread_t threads[P];
	data datum[P];
	pthread_barrier_t *main_barrier = (pthread_barrier_t*)malloc(sizeof(pthread_barrier_t));
	pthread_barrier_init(main_barrier, NULL, P);

	individual *current_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *next_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *merge_generation = (individual*) calloc(object_count, sizeof(individual));

	int i;
	for (i = 0; i < P; i++) {
		data d;

		d.id = i;
		d.P = P;

		d.objects = objects;
		d.object_count = object_count;
		d.sack_capacity = sack_capacity;
		d.generations_count = generations_count;

		d.barrier = main_barrier;
		d.current_generation = &current_generation;
		d.next_generation = &next_generation;
		d.merge_generation = &merge_generation;

		datum[i] = d;

		pthread_create(&threads[i], NULL, run_genetic_algorithm, &datum[i]);
	}

	for (i = 0; i < P; i++) {
		pthread_join(threads[i], NULL);
	}

	pthread_barrier_destroy(main_barrier);
	free(objects);

	return 0;
}
