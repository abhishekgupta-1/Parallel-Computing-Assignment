#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>


typedef struct pair_t{
    int first;
    int second;
} pair_t;

int main(int argc, char *argv[]){
    int myrank, size, len;
    char processor[100];
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Get_processor_name(processor, &len);
    //Enter you code here
    if (myrank == 0) // master code
    {
        printf("Enter number of items\n");
        int n, i;
        scanf("%d",&n);
        printf("Enter n value-weight pairs\n");
        pair_t * arr = (pair_t*)calloc(n, sizeof(pair-t));
        for (i = 0; i < n; i++)
            scanf("%d %d", &
        
        qsort(); // sort the pair according to value/weight ratio
        int busy[argc];
        memset(busy, '\0', argc*sizeof(char));
        busy[0] = 1;
        int idle = argc-1;

    }
    

    MPI_Finalize();
    return 0;
}
