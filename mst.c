#include <mpi.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>


#ifdef DEBUG
#define DBG(fmt, args...) fprintf (stdout, fmt, ##args)
#else
#define DBG(fmt, args...)
#endif

typedef enum { END_TAG, PBM_TAG, SOLVE_TAG, IDLE_TAG, BnB_TAG, DONE} tag_t;

int nextIdle(int *busy, int n, int* idle){
    int i;
    for (i = 0; i < n; i++){
        if (!busy[i]) {*idle = *idle - 1; busy[i] = 1;return i;}
    }
    return -1;
}

int **alloc_2d_int(int rows, int cols) {
    int *data = (int *)malloc(rows*cols*sizeof(int));
    int **array= (int **)malloc(rows*sizeof(int*));
    for (int i=0; i<rows; i++)
        array[i] = &(data[cols*i]);
    return array;
}

//partial solution
//n elem array - contains points to the next element in the cycle
//currCost occured
//bestCost up to date
//cur elem which has to take decision
//flag - which tells if this is the subtree which reported to master the current bestCost
//

//two new tags - 
//UPDATE BESTCOST_TAG, to inform master of the best cost
//BEST SOL - to inform of the best solution

// TODO
// upper_bound
// lower_bound
// branch  -- update curCost also

int n;
int **graph;


int upper_bound(int *auxSp){
    int last = auxSp[n+2];

    
}

int lower_bound(int *auxSp){

}



int main(int argc, char *argv[]){
    int i;
    int myrank, size, len;
    char processor[100];
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Get_processor_name(processor, &len);
    int nProc = size;
    DBG("Hello I am processor no. %d out of %d processors\n", myrank, size);
    //MPI TYPE    
    int count = 2;
    int array_of_blocklengths[] = { 1, 1 };
    MPI_Aint array_of_displacements[] = { offsetof( pair_t, value), 
        offsetof(pair_t, weight)};
    MPI_Datatype array_of_types[] = { MPI_INT, MPI_INT };
    MPI_Datatype tmp_type, MPI_PAIR;
    MPI_Aint lb, extent;
    MPI_Type_create_struct( count, array_of_blocklengths, array_of_displacements,
                            array_of_types, &tmp_type );
    MPI_Type_get_extent( tmp_type, &lb, &extent );
    MPI_Type_create_resized( tmp_type, lb, extent, &MPI_PAIR );
    MPI_Type_commit( &MPI_PAIR );
    
    //Enter you code here
    
    if (myrank == 0){ //master code
        //Receive Input
        int i,j;
        printf("Enter number of nodes\n");
        scanf("%d",&n);
        graph = alloc_2d_int(n,n);
        for (i=0;<n;i++)
            graph[i] = (int*)malloc(n*sizeof(int));
        printf("Enter adjacency matrix\n");
        for (i=0;i<n;i++)
            for (j=0;j<n;j++)
                scanf("%d", &graph[i][j]);
        DBG("MASTER : Input Received\n");
        //Send data to all other processors
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&graph[0][0], n*n, MPI_INT, 0, MPI_COM_WORLD);
        DBG("MASTER : Data Broadcast successfull!\n");
        int idle = nProc - 1;
        tag_t tag;
        int *busy = (int*)malloc(nProc*sizeof(int));
        for (i=0;i<nProc;i++) busy[i] = 0;
        busy[0] = 1;
        int dst = nextIdle(busy, nProc, &idle);
        int bestCost = MAX_INT;
        //find Initial Solution ----------------------- TODO
        int *initial_sP = (int*)malloc((n+4)*sizeof(int));
        for (i=0;i<n;i++)
            initial_sP[i] = 0;
        initial_sP[n] = 0; //curCost
        initial_sP[n+1] = bestCost; //bestCost
        initial_sP[n+2] = 0; //elem which has to take decision
        initial_sP[n+3] = 0;  //flag to check if to to dfs

        //Send initial problem to firstIdle processor
        int *bestSol = (int*)malloc((n+4)*sizeof(int));
        int *tempSol = (int*)malloc((n+4)*sizeof(int));
        int src;
        MPI_Send(initial_sP, n+4, MPI_INT, dst, PBM_TAG, MPI_COMM_WORLD);
        MPI_Status status;
        while (idle != nProc-1){
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            tag = status.MPI_TAG;
            src = status.MPI_SOURCE;
            if (tag == SOLVE_TAG) { 
                //receive best solution value and best solution
                MPI_Recv(tempSol, n+4, MPI_INT, src, tag, MPI_COMM_WORLD, &status);
                if (tempSol[n+1] < bestCost){
                    bestCost = tempSol[n+1];
                    for (i=0;i<n;i++)
                        bestSol[i] = tempSol[i];
                }
            }
            if (tag == IDLE_TAG) { //processor is idle
                MPI_Recv(&tag, 1, MPI_INT, src, tag, MPI_COMM_WORLD,  &status);
                DBG("Received idle tag from %d, total number of idle = %d\n", src, idle+1);
                idle++;
                busy[status.MPI_SOURCE] = 0;
            }
            if (tag == BnB_TAG) {
                if (high > bestSolVal) {
                    int total= ((nSlaves <= idle)?nSlaves:idle);
                    int *data = (int*)malloc((total+1)*sizeof(int)); //data[total] contains bestSolval
                    data[total] = bestSolVal; 
                    DBG("Assigning %d processors to %d : ", total, src);
                    for (i=0;i<total;i++){
                        data[i] = nextIdle(busy, nProc, &idle);
                        DBG("%d ",data[i]);
                    }
                    DBG("\n");
                    MPI_Send(data, total+1, MPI_INT, src, BnB_TAG, MPI_COMM_WORLD);
                    
                }
                else{
                    tag = DONE;
                    MPI_Send(&tag, 1, MPI_INT, src, DONE, MPI_COMM_WORLD);
                }
            }
        }
        for (i=1;i<nProc;i++){
            tag = END_TAG;
            MPI_Send(&tag, 1, MPI_INT, i, END_TAG, MPI_COMM_WORLD);
        }
        MPI_Finalize();
    }
    else {  //slave code
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        graph = alloc_2d_int(n,n);
        MPI_Bcast(&graph[0][0], n*n, MPI_INT, 0, MPI_COM_WORLD);
        list_t list; //stack for storing problems
        list.head = NULL; list.len = 0;
        int bestCost, currCost, flag;
        MPI_Status status;
        int src;
        while (1){
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            tag_t tag = status.MPI_TAG;
            src = status.MPI_SOURCE;
            if (tag == END_TAG){
                DBG("Processor %d: Program over!\n", myrank);
                MPI_Recv(&tag, 1, MPI_INT, src, tag, MPI_COMM_WORLD, &status);
                break;
            }
            if (tag == PBM_TAG) {
                DBG("Processor %d: %d asked me to work\n", myrank, src);
                int *auxSp = (int*)malloc((n+4)*sizeof(int)):
                //Receive subproblem
                MPI_Recv(auxSp, n+4, MPI_INT, src, tag, MPI_COMM_WORLD,&status);
                //Update flag, bestCost and currCost
                flag = auxSp[n+3];
                bestCost = auxSp[n+1];
                currCost = auxSp[n];
                //Insert the problem into list
                insert_into_list(&list, auxSp, n+4);
                while (!empty_list(&list)){
                    int *auxSp = remove_from_list(&list);
                    int low = lower_bound(auxSp);
                    DBG("Lower bound calculated by %d = %d\n",myrank, high);
                    if (low < bestCost){
                        int high = upper_bound(auxSp);
                        DBG("Upper bound calculated by %d = %d\n",myrank, high);
                        if (high < bestCost)  {
                            //Update bestCost here
                            bestCost = high;
                            auxSp[n+1] = high;
                            //Send bestSolVal and solution to the master
                            MPI_Send(auxSp, n+4, MPI_INT, 0, SOLVE_TAG, MPI_COMM_WORLD);
                        }
                        if (low != high){ 
                            int data[2];
                            data[0] = low;
                            data[1] = list.len;
                            MPI_Send(data, 2, MPI_INT, 0, BnB_TAG, MPI_COMM_WORLD);
                            int *assigned = (int*) malloc((list.len+1)*sizeof(int));
                            int total;
                            //Recive bestSolVal and processors assigned
                            MPI_Recv(assigned, list.len+1, MPI_INT, 0, BnB_TAG, MPI_COMM_WORLD, &status);
                            if (status.MPI_TAG == BnB_TAG){
                                MPI_Get_count(&status, MPI_INT, &total);
                                total--;
                                //assign received bestSolVal
                                bestCost = assigned[total];
                                auxSp[n+1] = bestCost;
                                //branch the problem
                                branch(&list, auxSp);
                                int *sp;
                                for (i=0;i<total;i++){
                                    //remove from list
                                    sp = remove_from_list(&list):
                                    //send bestSolVal and removed subproblem to the assigned node
                                    MPI_Send(sp, n+4, MPI_INT, assigned[i], PBM_TAG, MPI_COMM_WORLD);
                                }
                            }
                            else{
                                DBG("Received DONE_TAG by %d\n", myrank);
                            }
                            free(assigned);
                        }
                    }
                }
                tag = IDLE_TAG;
                MPI_Send(&tag, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD);
            }
        }
    }
    return 0;
}
