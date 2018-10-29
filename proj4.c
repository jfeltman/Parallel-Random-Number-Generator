#include "proj4.h"

void printMatrix(int m[][2], int rows, int cols) {
    int r, c;
    for(r = 0; r < rows; r++) {
        for(c = 0; c < cols; c++) {
            printf("%d ", m[r][c]);
        }
        printf("\n");
    }
}

void serial_baseline(int n, int x0, int output[]) {
    output[0] = x0;
    
    int i;
    for(i = 1; i < n; i++) {
        output[i] = (A * output[i-1] + B) % PRIME; 
    }
}

void unchanged_serial_matrix(int n, int x0, int output[]) {
    struct timeval start, end;

    gettimeofday(&start, NULL);    
    int M[2][2] = {
        { A, 0 },
        { B, 1 }
    };

    int mNext[2][2];
    memcpy(mNext, M, sizeof(mNext));

    int seedMatrix[1][2] = {
        { x0, 1 }
    };

    output[0] = x0;

    int i;
    for(i = 1; i < n; i++) {
        int product[1][2];
        multiplyMatrix(1, seedMatrix, mNext, product);
        output[i] = product[0][0];

        int tempMNext[2][2];
        multiplyMatrix(2, mNext, M, tempMNext);
        memcpy(mNext, tempMNext, sizeof(mNext));
    }
    gettimeofday(&end, NULL);
    long time = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
    serialRuntime += time;
}

void multiplyMatrix(int numRowsInLeft, int leftMatrix[][2], int rightMatrix[][2], int product[][2]) {
    int sum = 0;
   
    int a, b, c;
    for(a = 0; a < numRowsInLeft; a++) {
        for(b = 0; b < 2; b++) {
            for(c = 0; c < 2; c++) {
                sum = (sum + leftMatrix[a][c] * rightMatrix[c][b]) % PRIME;
            }
            product[a][b] = sum;
            sum = 0;
        }
    }
}

void serial_matrix(int n, int x0, int output[], int mOff[][2]) {
    int M[2][2] = {
        { A, 0 },
        { B, 1 }
    };

    int mNext[2][2];
    memcpy(mNext, mOff, sizeof(mNext));

    int seedMatrix[1][2] = { 
        { x0, 1 }
    };
    
    int i;
    for(i = 0; i < n; i++) {
        int product[1][2];
        multiplyMatrix(1, seedMatrix, mNext, product);
        output[i] = product[0][0];
    
        int tempMNext[2][2];
        multiplyMatrix(2, mNext, M, tempMNext);
        memcpy(mNext, tempMNext, sizeof(mNext));
    }
}

// P-Element Parallel Prefix
void matrix_parallel_prefix(int mLocal[][2], int outputM[][2]) {
    int local[2][2] = {
        { 1, 0 },
        { 0, 1 }
    };
    //memcpy(local, mLocal, sizeof(local));

    int global[2][2], recvGlobal[2][2];
    memcpy(global, mLocal, sizeof(global));

    int v = 1;
    int t, mate;

    for(t = 0; t < log2(p); t++) {
        mate = rank ^ v;
        v = v << 1;        

        // send 2x2 matrix
        MPI_Send(&(global[0][0]), 4, MPI_INT, mate, 0, MPI_COMM_WORLD);

        MPI_Status status;
        // recv 2x2 matrix
        MPI_Recv(&(recvGlobal[0][0]), 4, MPI_INT, mate, 0, MPI_COMM_WORLD, &status);

        // global *= recievedGlobal;
        int tempG[2][2];
        multiplyMatrix(2, global, recvGlobal, tempG);
        memcpy(global, tempG, sizeof(global));

        if (mate < rank) {
            // local = local * recvGlobal
            int tempL[2][2];
            multiplyMatrix(2, local, recvGlobal, tempL);
            memcpy(local, tempL, sizeof(local));
        }
    }
    
    // output local
    memcpy(outputM, local, sizeof(local));
}

// A, B, x0, p, PRIME all globals so every rank should have them already
void parallel_random_number_generator(int output[]) {
    struct timeval start, end;

    gettimeofday(&start, NULL);
    // Step 2. Init Matrix M and M_ZERO, xLocal
    int M[2][2] = { 
        { A, 0 },
        { B, 1 }
    };

    int M_ZERO[2][2] = { 
        { 1, 0 },
        { 0, 1 }
    };

    // Array that holds 2D arrays
    int **xLocal[n/p];
    int i;
    // fill xLocal with M
    for(i = 0; i < n/p; i++) {
        xLocal[i] = M;
    }

    // Debugging making sure local array was set up correctly
    //for(i = 0; i < n/p; i++) {
    //    printMatrix(xLocal[i], 2, 2);
    //}
    
    // Step 3. At each rank
    // Mlocal = MZERO
    // for i =0 to n/p-1
    //      Mlocal = Mlocal * Xlocal[i]
    int M_Local[2][2];
    memcpy(M_Local, M_ZERO, sizeof(M_Local));

    for(i = 0; i < n/p; i++) {
        int tempLocal[2][2];
        multiplyMatrix(2, M_Local, xLocal[i], tempLocal);
        memcpy(M_Local, tempLocal, sizeof(M_Local));
    }

    // debugging
    //printf("Result of step 3:\n");
    //printMatrix(M_Local, 2, 2);

    // Step 4. Run p-element parallel prefix, with each process providing its corresponding Mlocal as input
    // Output will be a 2x2 matrix M_off, which represents the prefix matrix product.
    int M_Off[2][2];
    matrix_parallel_prefix(M_Local, M_Off);

    // debugging
    //printf("RANK %d, step 4, parallel prefix\n", rank);
    //printMatrix(M_Off, 2, 2);
    //printf("-----------\n");
    
    // Step 5. At every process:
    //      Call serial_matrix(n/p) with 2 modificatoins:
    //      Output X from every process
    serial_matrix(n/p, initialSeed, output, M_Off);
    gettimeofday(&end, NULL);

    long time = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
    totalParallelRuntime += time;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    printf("my rank=%d\n",rank);
    printf("Rank=%d: number of processes =%d\n",rank,p);

    A = atoi(argv[1]);
    B = atoi(argv[2]);
    initialSeed = atoi(argv[3]);
    n = atoi(argv[4]);
    PRIME = atoi(argv[5]);
    
    printf("INPUTS = A: %d | B: %d | x0: %d | n: %d | PRIME: %d\n", A, B, initialSeed, n, PRIME);
 
    int baselineRandNums[n];
//    serial_baseline(n, initialSeed, baselineRandNums); 

    unchanged_serial_matrix(n, initialSeed, baselineRandNums);

    int i;
    printf("BASELINE SERIAL: ");
    for(i = 0; i < n; i++) {
        if(i % (n/p) == 0) {
            printf(" | ");
        }
        printf("%d ", baselineRandNums[i]);
    }
    printf("\n");
    
    int localOutput[n/p];
    parallel_random_number_generator(localOutput);

//    printf("RANK %d, PARALLEL RANDOM NUMBER GENERATOR: ", rank);
//    for(i = 0; i < n/p; i++) {
//        printf("%d ", localOutput[i]);
//    }
//    printf("\n"); 
       
    // Gather local outputs into rank 0
    int finalSequence[n];    
    struct timeval start, end;
    
    gettimeofday(&start, NULL);
    MPI_Gather(localOutput, n/p, MPI_INT, finalSequence, n/p, MPI_INT, 0, MPI_COMM_WORLD);
    gettimeofday(&end, NULL);

    long time = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
    totalParallelRuntime += time;

    printf("RANK: %d | Serial Time: %ld usec\n", rank, serialRuntime);
    printf("RANK: %d | Parallel Time: %ld usec\n", rank, totalParallelRuntime);

    printf("MPI GATHER\n");
    if(rank == 0) {
        printf("FINAL SEQUENCE: ");
        for(i = 0; i < n; i++) {
            printf("%d ", finalSequence[i]);
        }
        printf("\n");
    }


    MPI_Finalize();
    return 0;
}
