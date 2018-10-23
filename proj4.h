#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <sys/time.h>

#define PRIME 68111

int rank, p;
int A, B;

void multiplyMatrix(int numRowsInSeed, int seedMatrix[][2], int mNext[][2], int product[][2]);

void serial_baseline(int n, int x0, int output[]);
void serial_matrix(int n, int x0,  int output[]);
