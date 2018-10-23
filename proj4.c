#include "proj4.h"

void serial_baseline(int n, int x0, int output[]) {
    output[0] = x0;
    
    int i;
    for(i = 1; i < n; i++) {
        output[i] = (A * output[i-1] + B) % PRIME; 
    }
}

void multiplyMatrix(int numRowsInSeed, int seedMatrix[][2], int mNext[][2], int product[][2]) {
    int sum = 0;
   
    int a, b, c;
    for(a = 0; a < numRowsInSeed; a++) {
        for(b = 0; b < 2; b++) {
            for(c = 0; c < 2; c++) {
                sum += (seedMatrix[a][c] * mNext[c][b]) % PRIME;
            }
            product[a][b] = sum;
            sum = 0;
        }
    }
}

void serial_matrix(int n, int x0, int output[]) {

    output[0] = x0;

    int M[2][2] = {
        { A, 0 },
        { B, 1 }
    };

    int mNext[2][2];
    memcpy(mNext, M, sizeof(M));

    int seedMatrix[1][2] = { 
        { x0, 1 }
    };

    int i;
    for(i = 1; i < n; i++) {
        int product[1][2];
        multiplyMatrix(1, seedMatrix, mNext, product);
        output[i] = product[0][0];
    
        int tempMNext[2][2];
        multiplyMatrix(2, mNext, M, tempMNext);
        memcpy(mNext, tempMNext, sizeof(mNext));
    }
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    A = 2;
    B = 4;

    int baselineRandNums[10];
    int matrixRandNums[10];

    serial_baseline(10, 1, baselineRandNums); 
    serial_matrix(10, 1, matrixRandNums);

    int i;
    printf("BASELINE SERIAL: ");
    for(i = 0; i < 10; i++) {
        printf("%d ", baselineRandNums[i]);
    }
    printf("\n");
    
    printf("MATRIX SERIAL: ");
    for(i = 0; i < 10; i++) {
        printf("%d ", matrixRandNums[i]);
    }
    printf("\n");

    MPI_Finalize();
    return 0;
}
