#include <mpi.h>
#include <iostream>
#include <cstdio>

using namespace std;

// #define DEBUG

static int last_rank;

// Ref: https://stackoverflow.com/a/466278
static inline int upper_power_of_two(int v)
{
    // Fill all LSBs then add 1 to round up to next power of 2
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

// Read size of matrix_a and matrix_b (n, m, l) and whole data of matrixes from stdin
//
// n_ptr:     pointer to n
// m_ptr:     pointer to m
// l_ptr:     pointer to l
// a_mat_ptr: pointer to matrix a (a should be a continuous memory space for placing n * m elements of int)
// b_mat_ptr: pointer to matrix b (b should be a continuous memory space for placing m * l elements of int)
//
// If a_mat is 3x2 (nxm) matrix:
//   1 2
//   3 4
//   5 6
// Store the 1d array: 1, 3, 5, 2, 4, 6
//   m = 0 -> 1d_array[0] ~ [2]
//   m = 1 -> 1d_array[3] ~ [5]
//
// If b_mat is the same 3x2 (mxl) matrix as a_mat,
// Store the 1d array: 1, 2, 3, 4, 5, 6
//   m = 0 -> 1d_array[0] ~ [1]
//   m = 1 -> 1d_array[2] ~ [3]
//   m = 2 -> 1d_array[4] ~ [5]
//
void construct_matrices(int *n_ptr, int *m_ptr, int *l_ptr,
                        int **a_mat_ptr, int **b_mat_ptr)
{
    int n, m, l;
    int *a_mat, *b_mat;
    int world_rank, world_size;

#ifdef DEBUG
    double start_time, end_time;
#endif

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_rank == 0) {
        int remain_m, remain_slot, rank0_m, curr_m;
        int *curr_ms;
        int *part_ms;

#ifdef DEBUG
        start_time = MPI_Wtime();
#endif

        cin >> n >> m >> l;

        a_mat = new int[n * m];
        b_mat = new int[m * l];

        for (int _n = 0; _n < n; ++_n) {
            for (int _m = 0; _m < m; ++_m) {
                cin >> a_mat[_m * n + _n];
            }
        }

        for (int i = 0; i < m * l; ++i) {
            cin >> b_mat[i];
        }

#ifdef DEBUG
        printf("a_mat: ");
        for (int i = 0; i < n * m; ++i) {
            printf("%d ", a_mat[i]);
        }
        printf("\n");

        printf("b_mat: ");
        for (int i = 0; i < m * l; ++i) {
            printf("%d ", b_mat[i]);
        }
        printf("\n");
#endif

        // Split m:
        //   MPI process 1: MA(n * m1) * MB(m1 * l) = MC1(n * l)
        //   MPI process 2: MA(n * m2) * MB(m2 * l) = MC2(n * l)
        //   ...
        //   MPI process n: MA(n * mn) * MB(mn * l) = MCn(n * l)
        //   Result: MC1 + MC2 + ... + MCn
        //
        // m : 18
        // rank curr_m                   part_m
        // 0         0   (18/8)=2.xx ->       3
        // 1         3   (15/7)=2.xx ->       3
        // 2         6   (12/6)=2.0  ->       2
        // 3         8   (10/5)=2.0  ->       2
        // 4        10    (8/4)=2.0  ->       2
        // 5        12    (6/3)=2.0  ->       2
        // 6        14    (4/2)=2.0  ->       2
        // 7        16    (2/1)=2.0  ->       2
        //
        // m : 3
        // rank curr_m                   part_m
        // 0         0    (3/8)=0.xx ->       1
        // 1         1    (2/7)=0.xx ->       1
        // 2         2    (1/6)=0.xx ->       1
        // 3         3    (0/5)=0.0  ->       0
        // 4        10    (0/4)=0.0  ->       0
        // 5        12    (0/3)=0.0  ->       0
        // 6        14    (0/2)=0.0  ->       0
        // 7        16    (0/1)=0.0  ->       0
        //
        // m : 1907
        // rank curr_m                   part_m
        // 0         0 (1907/4)=476.xx ->   477
        // 1         1 (1430/3)=476.xx ->   477
        // 2         2  (953/2)=476.xx ->   477
        // 3         3  (476/1)=476.0  ->   476
        curr_m = 0;
        remain_m = m;
        remain_slot = world_size;
        curr_ms = new int[world_size];
        part_ms = new int[world_size];

        // rank 0
        rank0_m = (remain_m + remain_slot - 1) / remain_slot;
        curr_m += rank0_m;
        remain_m -= rank0_m;
        remain_slot -= 1;

        curr_ms[0] = 0;
        part_ms[0] = rank0_m;

        for (int rank = 1; rank < world_size; ++rank) {
            int part_m;

            part_m = (remain_m + remain_slot - 1) / remain_slot;

            if (part_m) {
                last_rank = rank;
            }

            curr_ms[rank] = curr_m;
            part_ms[rank] = part_m;

            curr_m += part_m;
            remain_m -= part_m;
            remain_slot -= 1;
        }

        // Sync data
        for (int rank = 1; rank < world_size; ++rank) {
            int curr_m, part_m;

            curr_m = curr_ms[rank];
            part_m = part_ms[rank];

            MPI_Send(&n, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
            MPI_Send(&part_m, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
            MPI_Send(&l, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
            MPI_Send(&last_rank, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);

            if (part_m) {
                MPI_Send(&a_mat[n * curr_m], n * part_m, MPI_INT, rank, 0, MPI_COMM_WORLD);
                MPI_Send(&b_mat[curr_m * l], part_m * l, MPI_INT, rank, 0, MPI_COMM_WORLD);
            }
        }

        *n_ptr = n;
        *m_ptr = rank0_m;
        *l_ptr = l;
        *a_mat_ptr = a_mat;
        *b_mat_ptr = b_mat;

        delete [] part_ms;
        delete [] curr_ms;

#ifdef DEBUG
        end_time = MPI_Wtime();
        printf("construct_matrices(0) running time: %lf Seconds\n", end_time - start_time);
#endif

        return;
    }

#ifdef DEBUG
    start_time = MPI_Wtime();
#endif

    // Recv
    MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&m, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&l, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&last_rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    *n_ptr = n;
    *m_ptr = m;
    *l_ptr = l;

    if (!m) {
        *a_mat_ptr = NULL;
        *b_mat_ptr = NULL;
        return;
    }

    a_mat = new int[n * m];
    b_mat = new int[m * l];

    MPI_Recv(a_mat, n * m, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(b_mat, m * l, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

#ifdef DEBUG
    end_time = MPI_Wtime();
    printf("construct_matrices running time: %lf Seconds\n", end_time - start_time);
#endif

    *a_mat_ptr = a_mat;
    *b_mat_ptr = b_mat;
}

// Just matrix multiplication (your should output the result in this function)
//
// n:     row number of matrix a
// m:     col number of matrix a / row number of matrix b
// l:     col number of matrix b
// a_mat: a continuous memory placing n * m elements of int
// b_mat: a continuous memory placing m * l elements of int
void matrix_multiply(const int n, const int m, const int l,
                     const int *a_mat, const int *b_mat)
{
    int world_rank, world_size, buddy_size, buddy_layer;
    int curr_start, remain_n, remain_slot;
    int *mul_mat, *_mul_mat;
    int a_base, b_base;
    std::string output;

    if (!m) {
        return;
    }

#ifdef DEBUG
    double start_time, end_time;
#endif

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

#ifdef DEBUG
    printf("[%d] n, m, l: %d %d %d %d\n", world_rank, n, m, l, last_rank);
    start_time = MPI_Wtime();
#endif

    mul_mat = new int[n * l];
    _mul_mat = new int[n * l];

    memset(mul_mat, 0, sizeof(int) * n * l);

    a_base = 0;
    b_base = 0;
    for (int _m = 0; _m < m; ++_m) {
        int m_base = 0;

        for (int _n = 0; _n < n; ++_n) {
            for (int _l = 0; _l < l; ++_l) {
                mul_mat[m_base + _l] += a_mat[a_base] * b_mat[b_base + _l];
            }

            m_base += l;
            a_base += 1;
        }
        
        b_base += l;
    }

#ifdef DEBUG
    end_time = MPI_Wtime();
    printf("matrix_multiply running time: %lf Seconds\n", end_time - start_time);
#endif

    buddy_size = upper_power_of_two(world_size);
    buddy_size >>= 1;
    buddy_layer = 0;

    while (buddy_size) {
        int buddy_rank, is_buddy_master;

        buddy_rank = world_rank ^ (1 << buddy_layer);
        is_buddy_master = !(world_rank & (1 << buddy_layer));

        if (buddy_rank > last_rank) {
            // Skip
            ++buddy_layer;
            buddy_size >>= 1;

            continue;
        }

        if (is_buddy_master) {
            // Recv

#ifdef DEBUG
            start_time = MPI_Wtime();
#endif

            MPI_Recv(_mul_mat,
                     n * l,
                     MPI_INT,
                     buddy_rank,
                     0,
                     MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);

            // Add

#ifdef DEBUG
            end_time = MPI_Wtime();
            printf("[%d] Recv running time: %lf Seconds\n", world_rank, end_time - start_time);
            start_time = MPI_Wtime();
#endif

            for (int _n = 0; _n < n; ++_n) {
                for (int _l = 0; _l < l; ++_l) {
                    mul_mat[_n * l + _l] += _mul_mat[_n * l + _l];
                }
            }

#ifdef DEBUG
            end_time = MPI_Wtime();
            printf("[%d] Add running time: %lf Seconds\n", world_rank, end_time - start_time);
#endif

        } else {
            // Send

#ifdef DEBUG
            start_time = MPI_Wtime();
#endif

            MPI_Send(mul_mat,
                     n * l,
                     MPI_INT,
                     buddy_rank,
                     0,
                     MPI_COMM_WORLD);

#ifdef DEBUG
            end_time = MPI_Wtime();
            printf("[%d] Send running time: %lf Seconds\n", world_rank, end_time - start_time);
#endif

            break;
        }

        ++buddy_layer;
        buddy_size >>= 1;
    }

    if (world_rank != 0) {
        delete [] _mul_mat;
        delete [] mul_mat;
        return;
    }

#ifdef DEBUG
    start_time = MPI_Wtime();
#endif

    // Output
    for (int _n = 0, i = 0; _n < n; ++_n) {
        for (int _l = 0; _l < l; ++_l, ++i) {
            output += std::to_string(mul_mat[i]);
            output += ' ';
        }
        output += '\n';
    }

    cout << output;

#ifdef DEBUG
    end_time = MPI_Wtime();
    printf("output running time: %lf Seconds\n", end_time - start_time);
#endif

    delete [] _mul_mat;
    delete [] mul_mat;
}

// Remember to release your allocated memory
void destruct_matrices(int *a_mat, int *b_mat)
{
    if (a_mat)
        delete [] a_mat;

    if (b_mat)
        delete [] b_mat;
}
