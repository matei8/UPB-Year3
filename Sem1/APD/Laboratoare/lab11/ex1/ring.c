#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
    int numtasks, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int recv_num;
    MPI_Request send_request, recv_request;
    MPI_Status status;
    int *flag = malloc(sizeof(int));

    if (rank == 0) {
        srand(42 + rank); // Seed the random number generator with a different seed for each process
        int rand_num = rand();
        printf("Before send: process with rank %d has the number %d.\n", rank, rand_num);

        // Send the number to the next process.
        MPI_Isend(&rand_num, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &send_request);
        MPI_Test(&send_request, flag, &status);

        // Receive the number from the last process.
        MPI_Irecv(&recv_num, 1, MPI_INT, numtasks - 1, 0, MPI_COMM_WORLD, &recv_request);
        MPI_Wait(&recv_request, &status);
        printf("After send: process with rank %d has the number %d.\n", rank, recv_num);

    } else if (rank == numtasks - 1) {
        // Receive the number from the previous process.
        MPI_Irecv(&recv_num, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &recv_request);
        MPI_Wait(&recv_request, &status);
        recv_num += 2;
        // Send the number to the first process.
        MPI_Isend(&recv_num, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &send_request);
        MPI_Wait(&send_request, &status);
        printf("After send: process with rank %d has the number %d.\n", rank, recv_num);

    } else {
        // Receive the number from the previous process.
        MPI_Irecv(&recv_num, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &recv_request);
        MPI_Wait(&recv_request, &status);
        recv_num += 2;
        // Send the number to the next process.
        MPI_Isend(&recv_num, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &send_request);
        MPI_Wait(&send_request, &status);
        printf("After send: process with rank %d has the number %d.\n", rank, recv_num);
    }

    MPI_Finalize();

    return 0;
}
