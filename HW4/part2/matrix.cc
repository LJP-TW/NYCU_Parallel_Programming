#include <mpi.h>
#include <iostream>
#include <cstdio>

using namespace std;

// #define DEBUG

static const int matrix_cmd_end  = 0;
static const int matrix_cmd_calc = 1;

#ifdef DEBUG
static int real_start_time;
#endif

static inline int bit_ffs(int v)
{
    return __builtin_ffs(v);
}

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
// Store the 1d array: 1, 2, 3, 4, 5, 6
//   m = 0 -> 1d_array[0] ~ [1]
//   m = 1 -> 1d_array[2] ~ [3]
//   m = 2 -> 1d_array[4] ~ [5]
//
// If b_mat is the same 3x2 (mxl) matrix as a_mat,
// Store the 1d array: 1, 3, 5, 2, 4, 6
//   m = 0 -> 1d_array[0] ~ [2]
//   m = 1 -> 1d_array[3] ~ [5]
//
void construct_matrices(int *n_ptr, int *m_ptr, int *l_ptr,
                        int **a_mat_ptr, int **b_mat_ptr)
{
    int n, m, l;
    int *a_mat, *b_mat;
    int world_rank, world_size;
    int buddy_size, buddy_layer;

#ifdef DEBUG
    double start_time, end_time;

    real_start_time = start_time = MPI_Wtime();
#endif

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_rank == 0) {
        int remain_m, remain_slot, rank0_m, curr_m;
        int *curr_ms;
        int *part_ms;

        cin >> n >> m >> l;

        a_mat = new int[n * m];
        b_mat = new int[m * l];

        for (int i = 0; i < n * m; ++i) {
            cin >> a_mat[i];
        }

        for (int _m = 0; _m < m; ++_m) {
            for (int _l = 0; _l < l; ++_l) {
                cin >> b_mat[_l * m + _m];
            }
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

    }

    // Sync n, m, l and matrix B
    buddy_size = upper_power_of_two(world_size);
    buddy_size >>= 1;

    if (world_rank == 0) {
        buddy_layer = bit_ffs(buddy_size);
    } else {
        buddy_layer = bit_ffs(world_rank);
    }

    buddy_layer -= 1;

#ifdef DEBUG
    printf("[%d]: buddy_size, buddy_layer: %d %d\n", world_rank, buddy_size, buddy_layer);
#endif

    while (buddy_layer >= 0) {
        int buddy_rank, is_buddy_master;

        buddy_rank = world_rank ^ (1 << buddy_layer);
        is_buddy_master = !(world_rank & (1 << buddy_layer));

        if (buddy_rank >= world_size) {
            break;
        }

        if (is_buddy_master) {
            // Send
            MPI_Send(&n, 1, MPI_INT, buddy_rank, 0, MPI_COMM_WORLD);
            MPI_Send(&m, 1, MPI_INT, buddy_rank, 0, MPI_COMM_WORLD);
            MPI_Send(&l, 1, MPI_INT, buddy_rank, 0, MPI_COMM_WORLD);
            MPI_Send(b_mat, m * l, MPI_INT, buddy_rank, 0, MPI_COMM_WORLD);

#ifdef DEBUG
            printf("[%d]: construct_matrices send to %d\n", world_rank, buddy_rank);
#endif

        } else {
            // Recv
            MPI_Recv(&n, 1, MPI_INT, buddy_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&m, 1, MPI_INT, buddy_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&l, 1, MPI_INT, buddy_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            a_mat = NULL;
            b_mat = new int[m * l];

            MPI_Recv(b_mat, m * l, MPI_INT, buddy_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

#ifdef DEBUG
            printf("[%d]: construct_matrices recv from %d\n", world_rank, buddy_rank);
#endif

        }

        --buddy_layer;
    }

    *n_ptr = n;
    *m_ptr = m;
    *l_ptr = l;
    *a_mat_ptr = a_mat;
    *b_mat_ptr = b_mat;

#ifdef DEBUG
    end_time = MPI_Wtime();
    printf("[%d] construct_matrices running time: %lf Seconds\n", world_rank, end_time - start_time);
#endif
}

void matrix_multiply_root(const int n, const int m, const int l,
                          const int *a_mat, const int *b_mat,
                          const int world_size,
                          int **mul_mat)
{
    int curr_n, waiting;
    int *_mul_mat;
    MPI_Request *requests;
    int requests_cnt;

    _mul_mat = new int[n * l];
    requests = new MPI_Request[world_size - 1];

    memset(_mul_mat, 0, sizeof(int) * n * l);
    curr_n = 0;
    waiting = 0;
    requests_cnt = 0;

    // Send one row of matrix A
    // Let other process calculate one row of result
    for (int rank = 1;
         rank < world_size && curr_n < n;
         ++rank, ++curr_n) {
        MPI_Send(&matrix_cmd_calc, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
        MPI_Send(&a_mat[curr_n * m], m, MPI_INT, rank, 0, MPI_COMM_WORLD);
        MPI_Irecv(&_mul_mat[curr_n * l],
                  l,
                  MPI_INT,
                  rank,
                  0,
                  MPI_COMM_WORLD,
                  &requests[rank - 1]);
        ++waiting;
        ++requests_cnt;
    }

    while (curr_n < n) {
        const int *_a_mat;
        int *_c_mat;
        int anyok = 0;
        int done_rank = 0;

        if (waiting) {
            // Check if any process sends result back
            MPI_Testany(requests_cnt, requests, &done_rank, &anyok, MPI_STATUS_IGNORE);
        }

        while (anyok) {
            --waiting;

            if (curr_n < n) {
                // Send one row of matrix A
                ++done_rank;                
                MPI_Send(&matrix_cmd_calc, 1, MPI_INT, done_rank, 0, MPI_COMM_WORLD);
                MPI_Send(&a_mat[curr_n * m], m, MPI_INT, done_rank, 0, MPI_COMM_WORLD);
                MPI_Irecv(&_mul_mat[curr_n * l],
                          l,
                          MPI_INT,
                          done_rank,
                          0,
                          MPI_COMM_WORLD,
                          &requests[done_rank - 1]);

                ++waiting;
                ++curr_n;
            }

            // Check if any process sends result back again
            MPI_Testany(requests_cnt, requests, &done_rank, &anyok, MPI_STATUS_IGNORE);
        }

        if (curr_n >= n) {
            break;
        }

        // Calculate one row of result in root node

        _a_mat = &a_mat[curr_n * m];
        _c_mat = &_mul_mat[curr_n * l];

        for (int _l = 0; _l < l; ++_l) {
            const int *_b_mat;
            int sum = 0;

            _b_mat = &b_mat[_l * m];

            for (int _m = 0; _m < m; ++_m) {
                sum += _a_mat[_m] * _b_mat[_m];
            }

            _c_mat[_l] = sum;
        }

        ++curr_n;
    }

    while (waiting) {
        int anyok = 0;
        int done_rank = 0;

        // Check if any process sends result back
        MPI_Testany(requests_cnt, requests, &done_rank, &anyok, MPI_STATUS_IGNORE);

        if (anyok) {
            --waiting;
        }
    }

    // Stop all the non-root processes
    for (int rank = 1; rank < world_size; ++rank) {
        MPI_Send(&matrix_cmd_end, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
    }

    *mul_mat = _mul_mat;

    delete [] requests;
}

void matrix_multiply_nonroot(const int m, const int l,
                             int *a_mat, const int *b_mat,
                             const int world_rank)
{
    int *mul_mat;

    mul_mat = new int[l];

    while (1) {
        int cmd;

        // Recv matrix A or terminating signal
        MPI_Recv(&cmd, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Is terminating signal?
        if (cmd == matrix_cmd_end) {
            break;
        }

        MPI_Recv(a_mat, m, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Calculate one row of result
        for (int _l = 0; _l < l; ++_l) {
            const int *_b_mat;
            int sum = 0;

            _b_mat = &b_mat[_l * m];

            for (int _m = 0; _m < m; ++_m) {
                sum += a_mat[_m] * _b_mat[_m];
            }

            mul_mat[_l] = sum;
        }

        // Send result back
        MPI_Send(mul_mat, l, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    delete [] mul_mat;
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
    int world_rank, world_size;
    int *mul_mat;
    std::string output;

#ifdef DEBUG
    double start_time, end_time;
#endif

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

#ifdef DEBUG
    end_time = MPI_Wtime();
    printf("[%d] matrix_multiply real start time: %lf Seconds\n", world_rank, end_time - real_start_time);
    printf("[%d] n, m, l: %d %d %d\n", world_rank, n, m, l);
#endif

    if (world_rank != 0) {
        // Calculate
        int *part_a_mat;

        part_a_mat = new int[m];

        matrix_multiply_nonroot(m, l, part_a_mat, b_mat, world_rank);

        delete [] part_a_mat;

        return;
    }

    // Calculate

#ifdef DEBUG
    start_time = MPI_Wtime();
#endif

    matrix_multiply_root(n, m, l, a_mat, b_mat, world_size, &mul_mat);
        
    // Output

#ifdef DEBUG
    end_time = MPI_Wtime();
    printf("matrix_multiply_root running time: %lf Seconds\n", end_time - start_time);
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
