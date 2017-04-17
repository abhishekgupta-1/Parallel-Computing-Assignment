#include <mpi.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include "list_header.h"


#ifdef DEBUG
#define DBG(fmt, args...) fprintf (stdout, fmt, ##args)
#else
#define DBG(fmt, args...)
#endif

int n;
int **graph;
 
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

int min( int *a, int ind){
    int temp=INT_MAX,i=0;
    for(i=0;i<n;i++){
        if(i!=ind && temp>a[i])
            temp=a[i];
    }
    return temp;
}

int minKey(int key[], int mstSet[]) {
   int min = INT_MAX, min_index;
   for (int v = 0; v < n; v++)
     if (mstSet[v] == 0 && key[v] < min)
         min = key[v], min_index = v;
   return min_index;
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
// branch  -- update curCost also


int upper_bound(int *auxSp) {
    int *key = (int*)malloc(n*sizeof(int));   // Key values used to pick minimum weight edge in cut
    int *mst = (int*)malloc(n*sizeof(int));  // To represent set of vertices not yet included in MST
    int i, j;
    // Initialize all keys as INFINITE
    for (i = 0; i < n; i++)
        key[i] = INT_MAX, mst[i] = 0;
    int count = 0;
    int inside = 0;
    for (i=0;i<n;i++)
        if (auxSp[i]!=-1) {
            inside = 1;
            mst[i] = 1;count++;
            int t = i;
            for (j=0;j<n;j++)
                if (t!=j && key[j] > graph[t][j])
                    key[j] = graph[t][j];
        }
    if (inside == 0) 
        key[0] = 0;
    int total = 0;
    for (i = 0; i < n-count; i++) {
        int u = minKey(key, mst);
        mst[u] = 1;
        total += key[u];
        for (int v = 0; v < n; v++)
            if (mst[v] == 0 && graph[u][v] <  key[v])
                key[v] = graph[u][v];
        
    }
    free(key);
    free(mst);
    return 2*(total+auxSp[n]);
}

int lower_bound(int *auxSp){
    int sum=auxSp[n],i=0;
    for(i=0;i<n;i++){
        if(auxSp[i]==-1){
            sum = sum + min(graph[i],i);
        }
    }
    return sum;
}

void branch(list_t *list, int *sol){
	int i=0,j=0;
        int found = 0;
	for(i=0;i<n;i++){
            if((sol[i]==-1)&&(i!=sol[n+2])){
                found = 1;
                int *newSubP = (int*)malloc((n+4)*sizeof(int));
                for (j=0;j<n+4;j++)
                        newSubP[j] = sol[j];
        	newSubP[sol[n+2]]=i;
       		newSubP[n] += graph[sol[n+2]][i]; //curCost
                //DBG("%d %d %d\n", i, newSubP[n], sol[n+2]);
                insert_into_list(list,newSubP,n);
            }
	}
        if (found == 0){
            sol[n] += graph[sol[n+2]][0]; //curCost
            printf("---------@@@@@@@@@@@@-----------------\n");
            insert_into_list(list,sol,n);
        }
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
    
    //Enter you code here
    
    if (myrank == 0){ //master code
        //Receive Input
        int i,j;
        printf("Enter number of nodes\n");
        scanf("%d",&n);
        graph = alloc_2d_int(n,n);
        printf("Enter adjacency matrix\n");
        for (i=0;i<n;i++)
            for (j=0;j<n;j++)
                scanf("%d", &graph[i][j]);
        DBG("MASTER : Input Received\n");
        //Send data to all other processors
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&graph[0][0], n*n, MPI_INT, 0, MPI_COMM_WORLD);
        DBG("MASTER : Data Broadcast successfull!\n");
        int idle = nProc - 1;
        tag_t tag;
        int *busy = (int*)malloc(nProc*sizeof(int));
        for (i=0;i<nProc;i++) busy[i] = 0;
        busy[0] = 1;
        int dst = nextIdle(busy, nProc, &idle);
        int bestCost = INT_MAX;
        //find Initial Solution ----------------------- TODO
        int *initial_sP = (int*)malloc((n+4)*sizeof(int));
        for (i=0;i<n;i++)
            initial_sP[i] = -1;
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
                int data[2];
                MPI_Recv(data,2,MPI_INT, src, tag, MPI_COMM_WORLD, &status);
                int low = data[0];
                int nSlaves = data[1];
                if (low < bestCost) {
                    int total= ((nSlaves <= idle)?nSlaves:idle);
                    int *data = (int*)malloc((total+1)*sizeof(int)); //data[total] contains bestSolval
                    data[total] = bestCost; 
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
        printf("Min Cost tour is : %d\n",bestCost);
        MPI_Finalize();
    }
    else {  //slave code
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        graph = alloc_2d_int(n,n);
        MPI_Bcast(&graph[0][0], n*n, MPI_INT, 0, MPI_COMM_WORLD);
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
                int *auxSp = (int*)malloc((n+4)*sizeof(int));
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
                    //DBG("New problem : \n");
                    //DBG("current cost : %d\n", auxSp[n]);
                    int low = lower_bound(auxSp);
                    DBG("Lower bound calculated by %d = %d\n",myrank, low);
                    if (low <= bestCost){
                        int high = upper_bound(auxSp);
                        DBG("Upper bound calculated by %d = %d\n",myrank, high);
                        if (high <= bestCost)  {
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
                                if (bestCost > assigned[total])
                                    bestCost = assigned[total];
                                auxSp[n+1] = bestCost;
                                //branch the problem
                                branch(&list, auxSp);
                                int *sp;
                                for (i=0;i<total;i++){
                                    //remove from list
                                    sp = remove_from_list(&list);
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
