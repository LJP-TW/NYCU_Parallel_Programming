#include <mpi.h>
#include <iostream>
#include <cstdio>

using namespace std;

// #define DEBUG

// Read size of matrix_a and matrix_b (n, m, l) and whole data of matrixes from stdin
//
// n_ptr:     pointer to n
// m_ptr:     pointer to m
// l_ptr:     pointer to l
// a_mat_ptr: pointer to matrix a (a should be a continuous memory space for placing n * m elements of int)
// b_mat_ptr: pointer to matrix b (b should be a continuous memory space for placing m * l elements of int)
//
// If a_mat is 3x3 matrix:
//   1 2 3
//   4 5 6
//   7 8 9
// Store the 1d array: 1, 2, 3, 4, 5, 6, 7, 8, 9
//
// If b_mat is the same 3x3 matrix as a_mat,
// Store the 1d array: 1, 4, 7, 2, 5, 8, 3, 6, 9
//
// With this arrangement the implementation of multiplication will be easier
//
void construct_matrices(int *n_ptr, int *m_ptr, int *l_ptr,
                        int **a_mat_ptr, int **b_mat_ptr)
{
    int n, m, l;
    int *a_mat, *b_mat;
    int world_rank, world_size;

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    if (world_rank == 0) {
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

        for (int rank = 1; rank < world_size; ++rank) {
            // Send
            MPI_Send(&n, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
            MPI_Send(&m, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
            MPI_Send(&l, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
            MPI_Send(a_mat, n * m, MPI_INT, rank, 0, MPI_COMM_WORLD);
            MPI_Send(b_mat, m * l, MPI_INT, rank, 0, MPI_COMM_WORLD);
        }

        *n_ptr = n;
        *m_ptr = m;
        *l_ptr = l;
        *a_mat_ptr = a_mat;
        *b_mat_ptr = b_mat;

        return;
    }

    // Recv
    MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&m, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&l, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    a_mat = new int[n * m];
    b_mat = new int[m * l];

    MPI_Recv(a_mat, n * m, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(b_mat, m * l, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    *n_ptr = n;
    *m_ptr = m;
    *l_ptr = l;
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
    int world_rank, world_size;
    int curr_start, remain_n, remain_slot;
    int *mul_mat;
    int *start_pos;
    int *loading;

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

#ifdef DEBUG
    printf("[%d] n, m, l: %d %d %d\n", world_rank, n, m, l);
#endif

    mul_mat = new int[n * l];
    start_pos = new int[world_size];
    loading = new int[world_size];
    curr_start = 0;
    remain_n = n;
    remain_slot = world_size;

    // n : 18
    //
    // i start_pos                  loading
    // 0         0   (18/8)=2.xx ->       3
    // 1         3   (15/7)=2.xx ->       3
    // 2         6   (12/6)=2.0  ->       2
    // 3         8   (10/5)=2.0  ->       2
    // 4        10    (8/4)=2.0  ->       2
    // 5        12    (6/3)=2.0  ->       2
    // 6        14    (4/2)=2.0  ->       2
    // 7        16    (2/1)=2.0  ->       2
    //
    // n : 3
    //
    // i start_pos                  loading
    // 0         0    (3/8)=0.xx ->       1
    // 1         1    (2/7)=0.xx ->       1
    // 2         2    (1/6)=0.xx  ->      1
    // 3         3    (0/5)=0.0  ->       0
    // 4        10    (0/4)=0.0  ->       0
    // 5        12    (0/3)=0.0  ->       0
    // 6        14    (0/2)=0.0  ->       0
    // 7        16    (0/1)=0.0  ->       0
    //
    // n : 3
    //
    // i start_pos                  loading
    // 0         0    (3/4)=0.xx ->       1
    // 1         1    (2/3)=0.xx ->       1
    // 2         2    (1/2)=0.xx ->       1
    // 3         3    (0/1)=0.0  ->       0
    for (int i = 0; i < world_size; ++i) {
        start_pos[i] = curr_start;
        loading[i] = (remain_n + remain_slot - 1) / remain_slot;
        curr_start += loading[i];
        remain_n -= loading[i];
        remain_slot -= 1;
    }

    // n, m, l: 3, 2 ,3
    //
    // a:
    //   1 2
    //   0 3
    //   4 5
    // -> 1 2 0 3 4 5
    //
    // b:
    //   1 2 1
    //   0 3 2
    // -> 1 0 2 3 1 2
    //
    for (int r = start_pos[world_rank], computed = 0;
         computed < loading[world_rank];
         ++r, ++computed) {
        for (int c = 0; c < l; ++c) {
            int sum = 0;

            for (int i = 0; i < m; ++i) {
                sum += a_mat[r * m + i] * b_mat[c * m + i];
            }

            mul_mat[r * l + c] = sum;
        }
    }

    // Use blocking-linear method to gather all the result.

    if (world_rank != 0) {
        if (loading[world_rank]) {

#ifdef DEBUG
            printf("[%d] Send\n", world_rank);
            printf("[%d] start_pos[%d] = %d\n", world_rank, world_rank, start_pos[world_rank]);
            printf("[%d] loading[%d] = %d\n", world_rank, world_rank, loading[world_rank]);
#endif

            // Send
            MPI_Send(&mul_mat[start_pos[world_rank] * l],
                     loading[world_rank] * l,
                     MPI_INT,
                     0,
                     0,
                     MPI_COMM_WORLD);
        }

        delete [] mul_mat;
        delete [] start_pos;
        delete [] loading;
        return;
    }

    for (int rank = 1; rank < world_size; ++rank) {
        if (!loading[rank]) {
            continue;
        }

#ifdef DEBUG
        printf("[%d] Recv (%d)\n", world_rank, rank);
        printf("[%d] start_pos[%d] = %d\n", world_rank, rank, start_pos[rank]);
        printf("[%d] loading[%d] = %d\n", world_rank, rank, loading[rank]);
#endif

        // Recv
        MPI_Recv(&mul_mat[start_pos[rank] * l],
                 loading[rank] * l,
                 MPI_INT,
                 rank,
                 0,
                 MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
    }

    // Output
    for (int _n = 0; _n < n; ++_n) {
        for (int _l = 0; _l < l; ++_l) {
            cout << mul_mat[_n * l + _l] << ' ';
        }
        cout << endl;
    }

    delete [] mul_mat;
    delete [] start_pos;
    delete [] loading;
}

// Remember to release your allocated memory
void destruct_matrices(int *a_mat, int *b_mat)
{
    delete [] a_mat;
    delete [] b_mat;   
}
