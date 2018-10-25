#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>

#define PRIME 68111

int rank, p, n;

// User inputs (from command line arguments)
int A, B, initialSeed;

void multiplyMatrix(int numRowsInSeed, int seedMatrix[][2], int mNext[][2], int product[][2]);
void serial_baseline(int n, int x0, int output[]);
//void serial_matrix(int n, int x0,  int output[]);
