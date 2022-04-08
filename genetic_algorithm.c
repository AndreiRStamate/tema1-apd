#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "genetic_algorithm.h"
#include <pthread.h>

#define min(a, b) a < b ? a : b

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int argc, char *argv[], int *P)
{
	FILE *fp;
	if (argc < 4) {
		fprintf(stderr, "Usage:\n\t./tema1_par in_file generations_count P\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	// initializing number of threads
	*P = atoi(argv[3]);

	return 1;
}

void print_data(data d) {
    for (int i = 0; i < d.object_count; i++) {
        printf("%d %d\n", d.objects[i].weight, d.objects[i].profit);
    }
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int object_count, int sack_capacity, int id, int P)
{
	int start = id * (double)object_count/ P;
	int end = min((id + 1) * (double)object_count / P, object_count);

	int weight;
	int profit;

	for (int i = start; i < end; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

void *run_genetic_algorithm(void *arg) {

	int start, end;

	data d = *(data*) arg;

	start = d.id * (double)d.object_count/ d.P;
	end = min((d.id + 1) * (double)d.object_count / d.P, d.object_count);

	int count, cursor;
	individual *tmp = NULL;

	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = start; i < end; ++i) {
		(*(d.current_generation))[i].fitness = 0;
		(*(d.current_generation))[i].chromosomes = (int*) calloc(d.object_count, sizeof(int));
		(*(d.current_generation))[i].chromosomes[i] = 1;
		(*(d.current_generation))[i].index = i;
		(*(d.current_generation))[i].chromosome_length = d.object_count;

		(*(d.next_generation))[i].fitness = 0;
		(*(d.next_generation))[i].chromosomes = (int*) calloc(d.object_count, sizeof(int));
		(*(d.next_generation))[i].index = i;
		(*(d.next_generation))[i].chromosome_length = d.object_count;
	}
	pthread_barrier_wait(d.barrier);

	// iterate for each generation
	for (int k = 0; k < d.generations_count; ++k) {
		cursor = 0;

		// compute fitness and sort by it
		compute_fitness_function(d.objects, (*(d.current_generation)), d.object_count, d.sack_capacity, d.id, d.P);
		pthread_barrier_wait(d.barrier);
		mergeSort(d);
		pthread_barrier_wait(d.barrier);

		// keep first 30% children (elite children selection)
		count = d.object_count * 3 / 10;
		start = d.id * count / d.P;
		end = min((d.id + 1) * count / d.P, d.object_count);
		for (int i = start; i < end; ++i) {
			copy_individual((*(d.current_generation)) + i, (*(d.next_generation)) + i);
		}
		pthread_barrier_wait(d.barrier);
		cursor = count;

		// mutate first 20% children with the first version of bit string mutation
		count = d.object_count * 2 / 10;
		start = d.id * count / d.P;
		end = min((d.id + 1) * count / d.P, d.object_count);
		for (int i = start; i < end; ++i) {
			copy_individual((*(d.current_generation)) + i, (*(d.next_generation)) + cursor + i);
			mutate_bit_string_1((*(d.next_generation)) + cursor + i, k);
		}
		pthread_barrier_wait(d.barrier);
		cursor += count;

		// mutate next 20% children with the second version of bit string mutation
		count = d.object_count * 2 / 10;
		start = d.id * count / d.P;
		end = min((d.id + 1) * count / d.P, d.object_count);
		for (int i = start; i < end; ++i) {
			copy_individual((*(d.current_generation)) + i + count, (*(d.next_generation)) + cursor + i);
			mutate_bit_string_2((*(d.next_generation)) + cursor + i, k);
		}
		pthread_barrier_wait(d.barrier);
		cursor += count;

		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		if (d.id == 0) {
			count = d.object_count * 3 / 10;

			if (count % 2 == 1) {
				copy_individual((*(d.current_generation)) + d.object_count - 1, (*(d.next_generation)) + cursor + count - 1);
				count--;
			}

			for (int i = 0; i < count; i += 2) {
				crossover((*(d.current_generation)) + i, (*(d.next_generation)) + cursor + i, k);
			}
		
			// switch to new generation
			tmp = (*(d.current_generation));
			(*(d.current_generation)) = (*(d.next_generation));
			(*(d.next_generation)) = tmp;
		}

		start = d.id * (double)d.object_count/ d.P;
		end = min((d.id + 1) * (double)d.object_count / d.P, d.object_count);
		for (int i = start; i < end; ++i) {
			(*(d.current_generation))[i].index = i;
		}
		pthread_barrier_wait(d.barrier);

		if (k % 5 == 0 && d.id == 0) { // all threads share the same data -> print only once
			print_best_fitness((*(d.current_generation)));
		}
		pthread_barrier_wait(d.barrier);
	}

	// compute fitness and sort by it
	compute_fitness_function(d.objects, (*(d.current_generation)), d.object_count, d.sack_capacity, d.id, d.P);
	pthread_barrier_wait(d.barrier);
	mergeSort(d);
	pthread_barrier_wait(d.barrier);
	
	if(d.id == 0) { // data is shared -> print once
		print_best_fitness((*(d.current_generation)));
	}

	if (d.id == 0) {
		// free resources for old generation
		 free_generation((*(d.current_generation)));
		 free_generation((*(d.next_generation)));

		// free resources
		free((*(d.current_generation)));
		free((*(d.next_generation)));
	}
	return NULL;
}

void merge(individual *source, int start, int mid, int end, individual *destination) {
	int iA = start;
	int iB = mid;

	for (int i = start; i < end; i++) {
		if (end == iB || (iA < mid && source[iA].fitness > source[iB].fitness)) {
			destination[i] = source[iA];
			iA++;
		} else {
			destination[i] = source[iB];
			iB++;
		}
	}
}

void mergeSort(data d)
{
	for (int width = 1; width < d.object_count; width = 2 * width) {
		int merges = d.object_count / (2 * width);
		int start = d.id * merges / d.P * 2 * width;
		int end = min((d.id + 1) * merges / d.P * 2 * width, d.object_count);

		for (int i = start; i < end; i = i + 2 * width) {
			merge((*(d.current_generation)), i, i + width, i + 2 * width, (*(d.merge_generation)));
		}
		pthread_barrier_wait(d.barrier);
		
		for (int i = 0; i < d.object_count; i++) { // replace current_generation with the merged values
			(*(d.current_generation))[i] = (*d.merge_generation)[i];
		}
		pthread_barrier_wait(d.barrier);
	}
}