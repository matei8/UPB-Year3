#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

// long cores = sysconf(_SC_NPROCESSORS_CONF);

#define NUM_THREADS 2

void *f1(void *arg) {
    long id = *(long *)arg;

    printf("Hello World sunt primul\n");

    pthread_exit(NULL);
}

void *f2(void *arg) {
    long id = *(long *)arg;
    printf("Hello World sunt al doilea\n");
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    pthread_t threads[NUM_THREADS];
    int r;
    long id = 0;
    void *status;
    long ids[NUM_THREADS];

    ids[id] = id;
    r = pthread_create(&threads[id], NULL, f1, &ids[id]);

    if (r) {
        printf("Eroare la crearea thread-ului %ld\n", id);
        exit(-1);
    }

    id = 1;
    ids[id] = id;
    r = pthread_create(&threads[id], NULL, f2, &ids[id]);

    if (r) {
        printf("Eroare la crearea thread-ului %ld\n", id);
        exit(-1);
    }

    for (id = 0; id < NUM_THREADS; id++) {
        r = pthread_join(threads[id], &status);

        if (r) {
            printf("Eroare la asteptarea thread-ului %ld\n", id);
            exit(-1);
        }
    }

    return 0;
}
