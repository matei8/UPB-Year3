#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int N;

void compareVectors(int *a, int *b) {
    // DO NOT MODIFY
    int i;
    for (i = 0; i < N; i++) {
        if (a[i] != b[i]) {
            printf("Sorted incorrectly\n");
            return;
        }
    }
    printf("Sorted correctly\n");
}

void displayVector(int *v) {
    // DO NOT MODIFY
    int i;
    for (i = 0; i < N; i++) {
        printf("%d ", v[i]);
    }
    printf("\n");
}

int cmp(const void *a, const void *b) {
    // DO NOT MODIFY
    int A = *(int *)a;
    int B = *(int *)b;
    return A - B;
}

int main(int argc, char *argv[]) {
    int rank, nProcesses;
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
    printf("Hello from %i/%i\n", rank, nProcesses);

    if (rank == 0) { // This code is run by a single process
        int *v = (int *)malloc(sizeof(int) * (nProcesses - 1));
        int *vQSort = (int *)malloc(sizeof(int) * (nProcesses - 1));
        int i;

        // Generate the vector v with random values
        srandom(42);
        for (i = 0; i < nProcesses - 1; i++)
            v[i] = random() % 200;
        N = nProcesses - 1;
        printf("Initial vector:\n");
        displayVector(v);

        // Make copy to check it against qsort
        for (i = 0; i < nProcesses - 1; i++)
            vQSort[i] = v[i];
        qsort(vQSort, nProcesses - 1, sizeof(int), cmp);

        // TODO: send the vector to rank == 1
        MPI_Send(v, nProcesses - 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

        // Receive the sorted vector from the last process
        MPI_Recv(v, nProcesses - 1, MPI_INT, nProcesses - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        printf("Sorted vector:\n");
        displayVector(v);
        compareVectors(v, vQSort);

        free(v);
        free(vQSort);
    } else {
        int *v = (int *)malloc(sizeof(int) * (nProcesses - 1));

        // Receive the vector from the previous process
        MPI_Recv(v, nProcesses - 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Sort the vector (each process sorts its own part)
        qsort(v, nProcesses - 1, sizeof(int), cmp);

        // Send the sorted vector to the next process
        if (rank < nProcesses - 1) {
            MPI_Send(v, nProcesses - 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
        } else {
            // If this is the last process, send the sorted vector back to rank 0
            MPI_Send(v, nProcesses - 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }

        free(v);
    }

    MPI_Finalize();
    return 0;
}