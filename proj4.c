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
    int M[2][2] = {
        { A, 0 },
        { B, 1 }
    };

    int mNext[2][2];
    memcpy(mNext, M, sizeof(M));

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
}

// Help for debugging
void serial_prefix(int A[], int output[], int n) {
    int sum = 0, i;

    for(i = 0; i < n; i++) {
        output[i] = sum + A[i];
        sum = output[i];
        printf("%d ", output[i]);
    }
    printf("\n");
}

// Make it p element prefix not n-element
// assume n = p
// pass in op (plus, mult, etc.)
void parallel_prefix(int A[]) {
   int local = A[rank];
   int global = A[rank];
   int receivedGlobal;

   int v = 1;
   int t, mate;

   for(t = 0; t < log2(p); t++) {
        mate = rank ^ v;
        v = v << 1;

        //Send global to mate
        MPI_Send(&global, 1, MPI_INT, mate, 0, MPI_COMM_WORLD);
        printf("RANK %d sent %d to %d\n", rank, global, mate);

        //Recieve recieveGlobal from mate
        MPI_Status status;
        MPI_Recv(&receivedGlobal, 1, MPI_INT, mate, 0, MPI_COMM_WORLD, &status);
        printf("RANK %d received %d from %d\n", rank, receivedGlobal, mate);

        global = global + receivedGlobal;
        
        if (mate < rank) {
            // replace '+' with op
            local = local + receivedGlobal;
        }
   }
   // output local;
   printf("RANK %d | Local = %d\n", rank, local);
}

// P-Element Parallel Prefix
void matrix_parallel_prefix(int mLocal[][2], int outputM[][2]) {
    int local[2][2];
    memcpy(local, mLocal, sizeof(local));

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

void printMatrix(int m[][2], int rows, int cols) {
    int r, c;
    for(r = 0; r < rows; r++) {
        for(c = 0; c < cols; c++) {
            printf("%d ", m[r][c]);
        }
        printf("\n");
    }
}

// A, B, x0, p, PRIME all globals so every rank should have them already
void parallel_random_number_generator() {
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
    int** xLocal[n/p];
    int i;
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
    printf("Result of step 3:\n");
    printMatrix(M_Local, 2, 2);

    // Step 4. Run p-element parallel prefix, with each process providing its corresponding Mlocal as input
    // , and matrix multiplaction operator. (Maybe use mod? not sure)
    // Output will be a 2x2 matrix M_off, which represents the prefix matrix product.
   

    // Step 5. At every process:
    //      Call serial_matrix(n/p) with 2 modificatoins:
    //              Init its Mnext = M_off (from step 4)
    //              Run its for loop from 0 to n -1
    //      Output X from every process

}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    printf("my rank=%d\n",rank);
    printf("Rank=%d: number of processes =%d\n",rank,p);

    char *tempA = argv[1];
    char *tempB = argv[2];
    char *tempInitialSeed = argv[3];
    A = atoi(tempA);
    B = atoi(tempB);
    initialSeed = atoi(tempInitialSeed);

    n = 12;

    int baselineRandNums[n];
    int matrixRandNums[n];

    serial_baseline(n, initialSeed, baselineRandNums); 
    serial_matrix(n, initialSeed, matrixRandNums);

    int i;
    printf("BASELINE SERIAL: ");
    for(i = 0; i < n; i++) {
        printf("%d ", baselineRandNums[i]);
    }
    printf("\n");
    
    printf("MATRIX SERIAL: ");
    for(i = 0; i < n; i++) {
        printf("%d ", matrixRandNums[i]);
    }
    printf("\n");

    int A[4] = { 1, 2, 3, 4 };
    int B[4];
    //serial_prefix(A, B, 4);

    //parallel_prefix(A);

    int local[2][2] = {
        { rank+1, rank+1 },
        { rank+1, rank+1 }
    };

    int out[2][2];

    matrix_parallel_prefix(local, out);
    
    printf("RANK %d\n", rank);
    printMatrix(out, 2, 2);

    //parallel_random_number_generator();

    MPI_Finalize();
    return 0;
}
