#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int *arr;
int array_size;
int num_threads;

struct timespec start, finish;
double elapsed;

typedef struct {
    int start;
    int end;
} ThreadData;

void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;

    // Storing start time
    clock_t start_time = clock();

    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds)
        ;
}

void *f(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    for (int i = data->start; i < data->end; i++) {
        arr[i] += 100;
    }
    delay(10);
    pthread_exit(NULL);
}

double min(double a, double b) {
    return a < b ? a : b;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Specificati dimensiunea array-ului si numarul de thread-uri\n");
        exit(-1);
    }

    array_size = atoi(argv[1]);
    num_threads = atoi(argv[2]);

    arr = malloc(array_size * sizeof(int));
    for (int i = 0; i < array_size; i++) {
        arr[i] = i;
    }

    for (int i = 0; i < array_size; i++) {
        printf("%d", arr[i]);
        if (i != array_size - 1) {
            printf(" ");
        } else {
            printf("\n");
        }
    }

    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    double chunk_size = (double)array_size / num_threads;

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = min(array_size, (i + 1) * chunk_size);
        pthread_create(&threads[i], NULL, f, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);

        pthread_join(threads[i], NULL);

        clock_gettime(CLOCK_MONOTONIC, &finish);
        elapsed = (finish.tv_sec - start.tv_sec);
        elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

        printf("elapsed time, thread no: %f, %d\n", elapsed, i);
    }

    for (int i = 0; i < array_size; i++) {
        printf("%d", arr[i]);
        if (i != array_size - 1) {
            printf(" ");
        } else {
            printf("\n");
        }
    }

    free(arr);
    return 0;
}