#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#pragma GCC optimize ("O3")

long long int hit;

double rand_get(double min, double max, unsigned int *seed)
{
    int r = rand_r(seed);
    return min + ((double)(r) / RAND_MAX) * (max - min);
}

void tf_estimate_pi(long long int tosses_cnt, int rank)
{
    unsigned int seed = getpid() ^ time(NULL) ^ (0xaed01498 << rank);

    for (long long int toss = 0; toss < tosses_cnt; toss ++) {
        double x, y, distance_squared;
        x = rand_get(-1, 1, &seed);
        y = rand_get(-1, 1, &seed);
        distance_squared = x * x + y * y;
        if (distance_squared <= 1)
            hit++;
    }
}

int main(int argc, char **argv)
{
    // --- DON'T TOUCH ---
    MPI_Init(&argc, &argv);
    double start_time = MPI_Wtime();
    double pi_result;
    long long int tosses = atoi(argv[1]);
    int world_rank, world_size;
    // ---

    // TODO: MPI init
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank > 0)
    {
        // TODO: MPI workers
        tosses /= world_size;
        tf_estimate_pi(tosses, world_rank);

        MPI_Send(&hit,
                 1,
                 MPI_LONG_LONG,
                 0,
                 0,
                 MPI_COMM_WORLD);
    }
    else if (world_rank == 0)
    {
        // TODO: non-blocking MPI communication.
        // Use MPI_Irecv, MPI_Wait or MPI_Waitall.
        MPI_Request requests[world_size];
        long long int holders[world_size];
        long long int part_tosses;

        part_tosses = (tosses / world_size) + (tosses % world_size);
        tf_estimate_pi(part_tosses, world_rank);

        for (int i = 1; i < world_size; ++i) {
            MPI_Irecv(&holders[i],
                      1,
                      MPI_LONG_LONG,
                      i,
                      0,
                      MPI_COMM_WORLD,
                      &requests[i]);
        }

        MPI_Waitall(world_size - 1, &requests[1], MPI_STATUSES_IGNORE);

        for (int i = 1; i < world_size; ++i) {
            hit += holders[i];
        }
    }

    if (world_rank == 0)
    {
        // TODO: PI result
        pi_result = 4 * hit / ((double)tosses);

        // --- DON'T TOUCH ---
        double end_time = MPI_Wtime();
        printf("%lf\n", pi_result);
        printf("MPI running time: %lf Seconds\n", end_time - start_time);
        // ---
    }

    MPI_Finalize();
    return 0;
}
