#include <mpi.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "list_header.h"


#ifdef DEBUG
#define DBG(fmt, args...) fprintf (stdout, fmt, ##args)
#else
#define DBG(fmt, args...)
#endif

typedef enum { END_TAG, PBM_TAG, SOLVE_TAG, IDLE_TAG, BnB_TAG, DONE} tag_t;
typedef struct pair_t{
    int value;
    int weight;
} pair_t;

pair_t * inp;
int n, bag_size;

int nextIdle(int *busy, int n, int* idle){
    int i;
    for (i = 0; i < n; i++){
        if (!busy[i]) {*idle = *idle - 1; busy[i] = 1;return i;}
    }
    return -1;
}

void branch(int *arr, int n, list_t * list){
    int upto;
    int i = 0;
    while (i<n && arr[i]!=-1)
        i++;
    upto = i;
    if (i != n){
        int *arr1, *arr2;
        arr1 = (int*)malloc(((n+1)*sizeof(int)));
        for (i=0;i<n+1;i++)
            arr1[i] = arr[i];
        arr2 = arr;
        arr1[upto] = 0;
        arr2[upto] = 1;
        insert_into_list(list, arr1, n+1);
        insert_into_list(list, arr2, n+1);
    }
}

int upper_bound(int *arr, int n){
    int total_val=0;
    int weight_used = 0;
    int i;
    int upto = 0;
    for (i=0; i<n && arr[i]!=-1; i++){
        if (arr[i] == 1){
            total_val += inp[i].value;
            weight_used += inp[i].weight;
        }
    }
    upto = i;
    if (weight_used>bag_size) return -1;
    int left = bag_size - weight_used;
    for (i=upto; i<n && weight_used<=bag_size; i++){
        total_val += inp[i].value;
        weight_used += inp[i].weight;
    }
    return total_val;
}

int lower_bound(int *arr, int n){
    int total_val=0;
    int weight_used = 0;
    int i;
    int upto = 0;
    for (i=0; i<n && arr[i]!=-1; i++){
        if (arr[i] == 1){
            total_val += inp[i].value;
            weight_used += inp[i].weight;
        }
    }
    upto = i;
    for (i=upto; i<n && weight_used<=bag_size; i++){
        total_val += inp[i].value;
        weight_used += inp[i].weight;
    }
    if (weight_used > bag_size){
        total_val -= inp[i-1].value;
    }
    return total_val;
}

int compar(const void *a, const void*b){
    pair_t * a1 = (pair_t*)a;
    pair_t * b1 = (pair_t*)b;
    double r1, r2;
    r1 = (double)a1->value/a1->weight;
    r2 = (double)b1->value/b1->weight;
    int res;
    if (r1>r2) res = 1;
    else res = -1;
    return -res;
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
        scanf("%d %d", &n, &bag_size);
        inp = (pair_t*)malloc(n*sizeof(pair_t));
        for (i=0;i<n;i++)
            scanf("%d %d", &(inp[i].value), &(inp[i].weight));
        DBG("Master : Input is complete\n");
        int *bestSol = (int*) malloc(sizeof(int)*(n+1));//bestSol[n] contains the bestSolVal;
        int *tempSol = (int*)malloc(sizeof(int)*(n+1));
        for (i=0;i<n;i++) tempSol[i] = bestSol[i] = -1;
        qsort(inp, n, sizeof(pair_t), compar);
        int pair[2];
        pair[0] = n; pair[1] = bag_size;
        for (i=1;i<nProc;i++){
            MPI_Send(pair, 2, MPI_INT, i, 1, MPI_COMM_WORLD);
            MPI_Send(inp, n, MPI_PAIR, i, 2, MPI_COMM_WORLD);
        }
        DBG("Master : Input sent to all other processes\n");
        int idle = nProc - 1;
        tag_t tag;
        int *busy = (int*)malloc(nProc*sizeof(int));
        for (i=0;i<nProc;i++)
            busy[i] = 0;
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
                MPI_Recv(tempSol, n+1, MPI_INT, src, tag, MPI_COMM_WORLD, &status);
                if (tempSol[n] >= bestSol[n]){
                    for (i=0;i<n+1;i++)
                        bestSol[i] = tempSol[i];
                    bestSolVal = bestSol[n];
                    DBG("Received best soln from %d, bestSolVal = %d\n", src, bestSol[n]);
                }
            }
            if (tag == IDLE_TAG) { //processor is idle
                MPI_Recv(&tag, 1, MPI_INT, src, tag, MPI_COMM_WORLD,  &status);
                DBG("Received idle tag from %d, total number of idle = %d\n", src, idle+1);
                idle++;
                busy[status.MPI_SOURCE] = 0;
            }
            if (tag == BnB_TAG) {
                int data[2];
                MPI_Recv(data, 2, MPI_INT, src, tag, MPI_COMM_WORLD, &status);
                int high = data[0], nSlaves = data[1];
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
        printf("Best result : %d and selections are\n", bestSolVal);
        for (i=0;i<n;i++)
            if (bestSol[i] == 1) printf("%d %d\n", inp[i].value, inp[i].weight);
        printf("\n");

        MPI_Finalize();
    }
    else {  //slave code
        //pair_t * inp;
        //int n, bag_size;
        int pair[2];
        MPI_Status status;
        MPI_Recv(pair, 2, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        n = pair[0]; bag_size = pair[1];
        inp = (pair_t*)malloc(n*sizeof(pair_t));
        MPI_Recv(inp, n, MPI_PAIR, 0, 2, MPI_COMM_WORLD, &status);
        DBG("Processor %d : Received input from master\n", myrank);
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
                int *axSol = (int*)malloc(sizeof(int)*(n+1));  //axSol[n] contains bestSol
                DBG("I am %d, %d asked me to work\n", myrank, src);
                MPI_Recv(axSol, n+1, MPI_INT, src, tag, MPI_COMM_WORLD, &status);
                bestSolVal = axSol[n];
                insert_into_list(&list, axSol, n+1);
                while (!empty_list(&list)){
                    int *auxSp = remove_from_list(&list);
                    int high = upper_bound(auxSp, n);
                    DBG("Upper bound calculated by %d = %d\n",myrank, high);
                    if (high >= bestSolVal){
                        int low = lower_bound(auxSp, n);
                        DBG("Lower bound calculated by %d = %d\n",myrank, low);
                        if (low >= bestSolVal)  {
                            auxSp[n] = low;
                            bestSolVal = low;
                            MPI_Send(auxSp, n+1, MPI_INT, 0, SOLVE_TAG, MPI_COMM_WORLD); //problem
                        }
                        if (low != high){ //problem is here
                            int data[2];
                            data[0] = high;
                            data[1] = list.len;
                            MPI_Send(data, 2, MPI_INT, 0, BnB_TAG, MPI_COMM_WORLD);
                            int *assigned = (int*) malloc((list.len+1)*sizeof(int));
                            MPI_Recv(assigned, list.len+1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                            if (status.MPI_TAG == BnB_TAG){
                                int total;
                                MPI_Get_count(&status, MPI_INT, &total);
                                bestSolVal = assigned[total-1]; total--;
                                branch(auxSp, n, &list);
                                for (i=0;i<total;i++){
                                    int *sp = remove_from_list(&list);
                                    sp[n] = bestSolVal;
                                    MPI_Send(sp, n+1, MPI_INT, assigned[i], PBM_TAG, MPI_COMM_WORLD);
                                    DBG("%d request %d to work!\n",myrank, assigned[i]);
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
