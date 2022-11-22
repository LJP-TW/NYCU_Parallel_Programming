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

    // TODO: use MPI_Gather
    long long int part_tosses;
    long long int hits[world_size];

    part_tosses = tosses / world_size;
    if (world_rank == 0) {
        part_tosses += (tosses % world_size);
    }

    tf_estimate_pi(part_tosses, world_rank);

    MPI_Gather(&hit, 1, MPI_LONG_LONG, hits, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    if (world_rank == 0) {
        for (int i = 1; i < world_size; ++i) {
            hit += hits[i];
        }

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
