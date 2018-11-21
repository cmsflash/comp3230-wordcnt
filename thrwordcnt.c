/*
 * Filename: thrwordcnt_3035233228.c
 * Student name: Shen Zhuoran
 * Student number: 3035233228
 * Development platform: Ubuntu 18.04.1 LTS, gcc 7.3.0
 * Compilation: gcc -pthread thrwordcnt_3035233228.c -o thrwordcnt -Wall
 * Remark: None
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>

#define TRUE 1
#define FALSE 0
#define MAX_LENGTH 116

const char* USAGE_GUIDE = (
		"Usage: ./thrwordcnt [number of workers] [number of buffers]"
		" [target plaintext file] [keyword file]"
);
const char* WORKER_COUNT_ERROR = (
	"The number of worker threads must be between 1 to 15"
);
const char* BUFFER_SIZE_ERROR = (
	"The number of buffers in task pool must be between 1 to 10"
);
const char* SEMAPHORE_FAILURE = "Semaphore failure.";
const char* THREAD_FAILURE = "Thread failure.";
const char* SENTINEL = "__sentinel__";

struct output {
	char keyword[MAX_LENGTH];
	unsigned int count;
};

struct output* results;
char* target_filename;
int next_result_id;
pthread_mutex_t pool_lock;
sem_t task_semaphore, vacancy_semaphore;

void assert(int successful, const char* message) {
	if (!successful) {
		printf("%s\n", message);
		exit(1);
	}
}

int get_semaphore_value(const sem_t* semaphore) {
	int value;
	sem_getvalue(&task_semaphore, &value);
	return value;
}

char* lower(char* input) {
	for (int i = 0; i < strlen(input); i++) {
		input[i] = tolower(input[i]);
	}
	return input;
}

void* search(struct output* result) {
	int count = 0;
	char* pos;
	char input[MAX_LENGTH];
	char* word = strdup((char*)result->keyword);

	lower(word);
	FILE* target_file;
	target_file = fopen(target_filename, "r");

    while (fscanf(target_file, "%s", input) != EOF) {
        lower(input);
		// Perfect match
        if (strcmp(input, word) == 0) {
			count++;
		}
		// Partial match
		else {
			pos = strstr(input, word);
			// word is substring of input
			if (pos != NULL) {
				// Character before word in input is letter, not match
				if ((pos - input > 0) && (isalpha(*(pos - 1)))) {
					continue; 
				}
				// Character after keyword in input is letter, not match
				else if (
					((pos - input + strlen(word) < strlen(input)))
					&& (isalpha(*(pos + strlen(word))))
				) {
					continue;
				}
				// Otherwise, is match
				else {
					count++;
				}
			}
		}
    }
    fclose(target_file);
	free(word);
	
	result->count = count;
    return NULL;
}

void* consume(void* arg) {
	int worker_id = *(int*)arg;
	int* keyword_count_pointer = (int*)malloc(sizeof(int));
	*keyword_count_pointer = 0;
	printf("Worker(%d) : Start up. Wait for task!\n", worker_id);
	while (TRUE) {
		sem_wait(&task_semaphore);
		pthread_mutex_lock(&pool_lock);
		int task_id = next_result_id;
		next_result_id++;
		pthread_mutex_unlock(&pool_lock);
		sem_post(&vacancy_semaphore);
		char* keyword = results[task_id].keyword;
		if (strcmp(keyword, SENTINEL) == 0) {
			break;
		} else {
			printf(
				"Worker(%d) : search for keyword \"%s\"\n", worker_id, keyword
			);
			search((void*)&results[task_id]);
			(*keyword_count_pointer)++;
		}
	}
	return (void*)keyword_count_pointer;
}

void produce() {
	sem_wait(&vacancy_semaphore);
	sem_post(&task_semaphore);
}


int main(int argc, char* argv[]) {
	// Argument parsing
	assert(argc == 5, USAGE_GUIDE);
	int worker_count = atoi(argv[1]);
	int buffer_size = atoi(argv[2]);
	target_filename = argv[3];
	char* keyword_filename = argv[4];
	assert(worker_count >= 1 && worker_count <= 15, WORKER_COUNT_ERROR);
	assert(buffer_size >= 1 && buffer_size <= 10, BUFFER_SIZE_ERROR);

	// Variable definition
	int line_count;
	FILE* keyword_file;

	// Variable initialization
	next_result_id = 0;
	keyword_file = fopen(keyword_filename, "r");
	fscanf(keyword_file, "%d", &line_count);
	results = (struct output*)malloc(
		(line_count + worker_count) * sizeof(struct output)
	);

	// Threading initialization
	pthread_t* threads = (pthread_t*)malloc(worker_count * sizeof(pthread_t));
	int* thread_ids = (int*)malloc(worker_count * sizeof(int));
	pthread_mutex_init(&pool_lock, NULL);
	assert(sem_init(&task_semaphore, 0, 0) != -1, SEMAPHORE_FAILURE);
	assert(
		sem_init(&vacancy_semaphore, 0, (unsigned int)buffer_size) != -1,
		SEMAPHORE_FAILURE
	);
	for (int i = 0; i < worker_count; i++) {
		thread_ids[i] = i;
		pthread_create(&threads[i], NULL, consume, (void*)&thread_ids[i]);
	}
	
	// Sentinal contruction
	for (int i = line_count; i < line_count + worker_count; i++) {
		strcpy(results[i].keyword, SENTINEL);
	}
	
	// Producer
	for (int i = 0; i < line_count; i++) {
		fscanf(keyword_file, "%s", results[i].keyword);
		produce();
	}
	fclose(keyword_file);
	for (int i = 0; i < worker_count; i++) {
		produce();
	}
	
	// Thread joining and result collection
	for (int i = 0; i < worker_count; i++) {
		void* keyword_count_pointer;
		pthread_join(threads[i], &keyword_count_pointer);
		printf(
			"Worker thread %d has terminated and completed %d tasks.\n",
			i, *(int*)keyword_count_pointer
		);
	}
	for (int i = 0; i < line_count; i++) {
		printf("%s : %d\n", results[i].keyword, results[i].count);
	}
	
	free(results);

	// Threading finalization
	pthread_mutex_destroy(&pool_lock);
	sem_destroy(&task_semaphore);
	sem_destroy(&vacancy_semaphore);
	return 0;
}
