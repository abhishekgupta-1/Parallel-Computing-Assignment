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

typedef enum { END_TAG, PBM_TAG, SOLVE_TAG, IDLE_TAG, BnB_TAG, DONE} tag_t;

int n;
int **graph;

int nextIdle(int *busy, int n, int* idle){
    int i;
    for (i = 0; i < n; i++){
        if (!busy[i]) {*idle = *idle - 1; busy[i] = 1;return i;}
    }
    return -1;
}

int min( int *a, int ind){
    int temp=INT_MAX,i=0;
    for(i=0;i<n;i++){
        if(i!=ind && temp>a[i])
            temp=a[i];
    }
    return temp;
}

int lower_bound(int *auxSp){
    int sum=auxSp[n+1],i=0;
    for(i=0;i<n;i++){
        if(auxSp[i]==-1){
            sum = sum + min(graph[i],i);
        }
    }
    return sum;
}

void branch(list_t *list, int *auxSp){
    int i=0,j=0;
    int found = 0;
    int last = auxSp[n+2];
    for(i=0;i<n;i++){
        if((auxSp[i]==-1) && (i!=last)){
            found = 1;
            int *newSubP = (int*)malloc((n+4)*sizeof(int));
            for (j=0;j<n+4;j++)
                newSubP[j] = auxSp[j];
            newSubP[last]=i;
            newSubP[n+1] += graph[last][i]; //curCost
            newSubP[n+2] = i;
            insert_into_list(list,newSubP,n);
        }
    }
    if (found == 0){
        auxSp[last] = 0;
        auxSp[n+1] += graph[last][0]; //curCost
        auxSp[n+2] = -1;
        auxSp[n+3] = 1;
        DBG("I am the chosen one! : %d\n",auxSp[n+1]);
        insert_into_list(list,auxSp,n);
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
    DBG("Hello I am processor no. %d out of %d processors\n", myrank, size);
    
    //Enter you code here
    
    if (myrank == 0){ //master code
        //Receive Input
        int i,j;
        scanf("%d",&n);
        int *data = (int*)malloc(n*n*sizeof(int));
        int **graph = (int**)malloc(n*sizeof(int*));
        for (i=0;i<n;i++)
            graph[i] = &(data[i*n]);
        for (i=0;i<n;i++)
            for (j=0;j<n;j++)
                scanf("%d",&graph[i][j]);
        MPI_Bcast(&n,1,MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(data,n*n,MPI_INT, 0, MPI_COMM_WORLD);
        DBG("INPUT BROADCASTED\n");
        int idle = nProc - 1;
        tag_t tag;
        int *busy = (int*)malloc(nProc*sizeof(int));
        for (i=0;i<nProc;i++) busy[i] = 0;
        busy[0] = 1;
        int dst = nextIdle(busy, nProc, &idle);
        //initial Sp
        int bestCost = INT_MAX;
        int *pb = (int*)malloc((n+4)*sizeof(int));
        for (i=0;i<n;i++)
            pb[i] = -1;
        pb[n] = bestCost; //bestCost;
        pb[n+1] = 0; //currCost;
        pb[n+2] = 0; //last to take decision
        pb[n+3] = 0; //flag to check if problem has reached the leaf
        MPI_Send(pb,n+4,MPI_INT,dst,PBM_TAG,MPI_COMM_WORLD);
        while (idle != nProc-1){
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            tag = status.MPI_TAG;
            int src = status.MPI_SOURCE;
            DBG("MASTER : Received %d from %d\n",tag, src);
            if (tag == SOLVE_TAG) { //receive best solution value and best solution
               MPI_Recv(pb,n+4,MPI_INT,src,tag,MPI_COMM_WORLD,&status);
               bestCost = pb[n+1];
            }
            if (tag == IDLE_TAG) { //processor is idle
                DBG("Received idle tag from %d, total number of idle = %d\n", src, idle+1);
                MPI_Recv(&tag, 1, MPI_INT, src, tag, MPI_COMM_WORLD,  &status);
                idle++;
                busy[status.MPI_SOURCE] = 0;
            }
            if (tag == BnB_TAG) {
                int inp[2];
                MPI_Recv(inp,2,MPI_INT,src,tag,MPI_COMM_WORLD,&status);
                int low = inp[0];
                int nSlaves = inp[1];
                if (low < bestCost) {
                    int total= ((nSlaves <= idle)?nSlaves:idle);
                    int *data = (int*)malloc((total+1)*sizeof(int)); //data[total] contains bestCost
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
                    MPI_Send(&bestCost, 1, MPI_INT, src, DONE, MPI_COMM_WORLD);
                }
            }
        }
        for (i=1;i<nProc;i++){
            tag = END_TAG;
            MPI_Send(&tag, 1, MPI_INT, i, END_TAG, MPI_COMM_WORLD);
        }
        printf("Minimum cost is %d\n",bestCost);
        printf("Path is : ");
        int temp = pb[0];
        printf("0 ");
        while (temp!=0){
            printf("%d ",temp);
            temp = pb[temp];
        }
        printf("0\n");
        MPI_Finalize();
    }
    else {  //slave code
        int i,j;
        MPI_Bcast(&n,1,MPI_INT, 0, MPI_COMM_WORLD);
        int *data = (int*)malloc(n*n*sizeof(int));
        graph = (int**)malloc(n*sizeof(int*));
        for (i=0;i<n;i++)
            graph[i] = &(data[i*n]);
        MPI_Bcast(data,n*n,MPI_INT, 0, MPI_COMM_WORLD);
        list_t list;
        list.head = NULL; list.len = 0;
        int bestCost;
        MPI_Status status;
        while (1){
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            tag_t tag = status.MPI_TAG;
            int src = status.MPI_SOURCE;
            if (tag == END_TAG){
                DBG("Processor %d : Program over!\n", myrank);
                MPI_Recv(&tag, 1, MPI_INT, src, tag, MPI_COMM_WORLD, &status);
                break;
            }
            if (tag == PBM_TAG) {
                DBG("I am %d, %d asked me to work\n", myrank, src);
                //Receive subproblem
                int *spm = (int*)malloc((n+4)*sizeof(int));
                MPI_Recv(spm,n+4,MPI_INT,src,tag,MPI_COMM_WORLD,&status);
                bestCost= spm[n]; 
                insert_into_list(&list,spm,n);
                while (!empty_list(&list)){
                    int *auxSp = remove_from_list(&list);
                    if (auxSp[n+3] == 1){
                        DBG("Chosen one speaks again! : %d\n",auxSp[n+1]);
                        if (auxSp[n+1] < bestCost){
                            bestCost = auxSp[n+1];
                            MPI_Send(auxSp, n+4, MPI_INT, 0, SOLVE_TAG, MPI_COMM_WORLD);
                        }
                        DBG("Continuing\n");
                        continue;
                    }
                    int low = lower_bound(auxSp);
                    DBG("Lower bound calculated by %d = %d\n",myrank, low);
                    if (low < bestCost){
                        //int high = higher_bound(auxSp, n);
                        //DBG("Lower bound calculated by %d = %d\n",myrank, low);
                        //if (low > bestSolVal)  {
                            ////Update bestSolVal here
                            ////Send bestSolVal and solution to the master
                        //}
                        int data[2];
                        data[0] = low;
                        data[1] = list.len;
                        MPI_Send(data, 2, MPI_INT, 0, BnB_TAG, MPI_COMM_WORLD);
                        int *assigned = (int*) malloc((list.len+1)*sizeof(int));
                        //Recive bestCost and processors assigned
                        MPI_Recv(assigned,list.len+1,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD, &status);
                        //get total
                        int total;
                        MPI_Get_count(&status,MPI_INT,&total);
                        bestCost = assigned[total-1];
                        total--;
                        if (status.MPI_TAG == BnB_TAG){
                            //branch the problem
                            branch(&list, auxSp);
                            //distribute the work
                            for (i=0;i<total;i++){
                                //remove from list
                                int *sp = remove_from_list(&list);
                                sp[n] = bestCost;
                                //send removed subproblem to the assigned node
                                MPI_Send(sp, n+4, MPI_INT, assigned[i], PBM_TAG, MPI_COMM_WORLD);
                            }
                        }
                        else{
                            DBG("Received DONE_TAG by %d\n", myrank);
                        }
                        free(assigned);
                    }
                }
                tag = IDLE_TAG;
                MPI_Send(&tag, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD);
            }
        }
        MPI_Finalize();
    }
    return 0;
}
