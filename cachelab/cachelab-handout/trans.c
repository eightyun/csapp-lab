/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

char transpose1[] = "Transpose 1";
void transpose11(int M, int N, int A[N][M], int B[M][N])
{
    int i , j;
    int i1;
    int val1,val2,val3,val4,val5,val6,val7,val8;
    for(i = 0 ; i < 32 ; i += 8)
        for(j = 0 ; j < 32 ; j += 8)
            for(i1 = i ; i1 < i + 8 ; i1++)
            {
                val1 = A[i1][j + 0];
                val2 = A[i1][j + 1];
                val3 = A[i1][j + 2];
                val4 = A[i1][j + 3];
                val5 = A[i1][j + 4];
                val6 = A[i1][j + 5];
                val7 = A[i1][j + 6];
                val8 = A[i1][j + 7];
                B[j + 0][i1] = val1;
                B[j + 1][i1] = val2;
                B[j + 2][i1] = val3;
                B[j + 3][i1] = val4;
                B[j + 4][i1] = val5;
                B[j + 5][i1] = val6;
                B[j + 6][i1] = val7;
                B[j + 7][i1] = val8;
            }
}

char transpose2[] = "Transpose 2" ;
void transpose22(int M , int N , int A[N][M] , int B[M][N])
{
    int i, j, x, y;
    int x1, x2, x3, x4, x5, x6, x7, x8;
    for (i = 0; i < N; i += 8)
        for (j = 0; j < M; j += 8)
        {
            for (x = i; x < i + 4; ++x)
            {
                x1 = A[x][j]; x2 = A[x][j+1]; x3 = A[x][j+2]; x4 = A[x][j+3];
                x5 = A[x][j+4]; x6 = A[x][j+5]; x7 = A[x][j+6]; x8 = A[x][j+7];

                B[j][x] = x1; B[j+1][x] = x2; B[j+2][x] = x3; B[j+3][x] = x4;
                B[j][x+4] = x5; B[j+1][x+4] = x6; B[j+2][x+4] = x7; B[j+3][x+4] = x8;
            }
            for (y = j; y < j + 4; ++y)
            {
                x1 = A[i+4][y]; x2 = A[i+5][y]; x3 = A[i+6][y]; x4 = A[i+7][y];
                x5 = B[y][i+4]; x6 = B[y][i+5]; x7 = B[y][i+6]; x8 = B[y][i+7];

                B[y][i+4] = x1; B[y][i+5] = x2; B[y][i+6] = x3; B[y][i+7] = x4;
                B[y+4][i] = x5; B[y+4][i+1] = x6; B[y+4][i+2] = x7; B[y+4][i+3] = x8;
            }
            for (x = i + 4; x < i + 8; ++x)
            {
                x1 = A[x][j+4]; x2 = A[x][j+5]; x3 = A[x][j+6]; x4 = A[x][j+7];
                B[j+4][x] = x1; B[j+5][x] = x2; B[j+6][x] = x3; B[j+7][x] = x4;
            }
        }
}

char transpose3[] = "Transpose 3" ;
void transpose33(int M , int N , int A[N][M] , int B[M][N])
{
    int i, j, v1, v2, v3, v4, v5, v6, v7, v8;
    int n = N / 8 * 8;
    int m = M / 8 * 8;
    for (j = 0; j < m; j += 8)
        for (i = 0; i < n; ++i)
        {
            v1 = A[i][j];
            v2 = A[i][j+1];
            v3 = A[i][j+2];
            v4 = A[i][j+3];
            v5 = A[i][j+4];
            v6 = A[i][j+5];
            v7 = A[i][j+6];
            v8 = A[i][j+7];

            B[j][i] = v1;
            B[j+1][i] = v2;
            B[j+2][i] = v3;
            B[j+3][i] = v4;
            B[j+4][i] = v5;
            B[j+5][i] = v6;
            B[j+6][i] = v7;
            B[j+7][i] = v8;
        }

    for (i = n; i < N; ++i)
        for (j = m; j < M; ++j)
        {
            v1 = A[i][j];
            B[j][i] = v1;
        }

    for (i = 0; i < N; ++i)
        for (j = m; j < M; ++j)
        {
            v1 = A[i][j];
            B[j][i] = v1;
        }

    for (i = n; i < N; ++i)
        for (j = 0; j < M; ++j)
        {
            v1 = A[i][j];
            B[j][i] = v1;
        }
}

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if(M == 32)
        return transpose11(32 , 32 , A , B);

    else if(M == 64)
        return transpose22(64 , 64 , A , B);

    else if(M == 61)
        return transpose33(M , N , A , B);
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0 ; i < N ; i++)
        for (j = 0 ; j < M ; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
                return 0;
        }

    return 1;
}

