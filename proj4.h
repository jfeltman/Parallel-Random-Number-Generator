#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>

// MPI Globals
int rank, p;

// User inputs (from command line arguments)
int A, B, initialSeed, n, PRIME;

// Timing Globals
long serialRuntime, totalParallelRuntime;

void printMatrix(int m[][2], int rows, int cols);
void multiplyMatrix(int numRowsInLeft, int leftMatrix[][2], int rightMatrix[][2], int product[][2]);
void serial_baseline(int n, int x0, int output[]);
void unchanged_serial_matrix(int n, int x0, int output[]);
void serial_matrix(int n, int x0, int output[], int mOff[][2]);
void matrix_parallel_prefix(int mLocal[][2], int outputM[][2]);
void parallel_random_number_generator(int output[]);
