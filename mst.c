#include <mpi.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>


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
        int idle = nProc - 1;
        tag_t tag;
        int *busy = (int*)malloc(nProc*sizeof(int));
        for (i=0;i<nProc;i++) busy[i] = 0;
        busy[0] = 1;
        int dst = nextIdle(busy, nProc, &idle);
        int total_weight = 0, bestSolVal = 0;
        for (i=0;i<n && total_weight<=bag_size;i++){
            total_weight += inp[i].weight;
            bestSolVal += inp[i].value;
            bestSol[i] = 1;
        }
        if (total_weight > bag_size){ bestSol[i-1] = 0; bestSolVal -= inp[i-1].value;}
        bestSol[n] = bestSolVal;
        tempSol[n] = bestSolVal;
        MPI_Send(tempSol, n+1, MPI_INT, dst, PBM_TAG, MPI_COMM_WORLD);
        while (idle != nProc-1){
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            tag = status.MPI_TAG;
            int src = status.MPI_SOURCE;
            if (tag == SOLVE_TAG) { //receive best solution value and best solution
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
        list_t list;
        list.head = NULL; list.len = 0;
        int bestSolVal;
        //axSp is same as axSol
        while (1){
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            tag_t tag = status.MPI_TAG;
            int src = status.MPI_SOURCE;
            if (tag == END_TAG){
                DBG("Processor %d : Game over!\n", myrank);
                MPI_Recv(&tag, 1, MPI_INT, src, tag, MPI_COMM_WORLD, &status);
                break;
            }
            if (tag == PBM_TAG) {
                DBG("I am %d, %d asked me to work\n", myrank, src);
                //Receive subproblem
                //Update bestSolVal
                //Insert the problem into list
                while (!empty_list(&list)){
                    int *auxSp = remove_from_list(&list);
                    int high = upper_bound(auxSp);
                    DBG("Upper bound calculated by %d = %d\n",myrank, high);
                    if (high > bestSolVal){
                        int low = lower_bound(auxSp, n);
                        DBG("Lower bound calculated by %d = %d\n",myrank, low);
                        if (low > bestSolVal)  {
                            //Update bestSolVal here
                            //Send bestSolVal and solution to the master
                        }
                        if (low != high){ 
                            int data[2];
                            data[0] = high;
                            data[1] = list.len;
                            MPI_Send(data, 2, MPI_INT, 0, BnB_TAG, MPI_COMM_WORLD);
                            int *assigned = (int*) malloc((list.len+1)*sizeof(int));
                            //Recive bestSolVal and processors assigned
                            //assign received bestSolVal
                            if (status.MPI_TAG == BnB_TAG){
                                int total; //number of processor assigned
                                //branch the problem
                                for (i=0;i<total;i++){
                                    //remove from list
                                    //send bestSolVal and removed subproblem to the assigned node
                                    MPI_Send(sp, n+1, MPI_INT, assigned[i], PBM_TAG, MPI_COMM_WORLD);
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
