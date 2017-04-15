#include <stdlib.h>
#include <stdio.h>
#include <qsort.h>
#include <mpi.h>
#include <stddef.h>


typedef enum { END_TAG, PBM_TAG, SOLVE_TAG, IDLE_TAG, BnB_TAG, DONE} flag_t;

struct node{
    int *currSol;
    
};



typedef struct pair_t{
    int value;
    int weight;
} pair_t;

int firstIdle(int *busy, int n, int* idle){
    int i;
    for (i = 0; i < n; i++){
        if (!busy[i]) {*idle = *idle - 1; busy[i] = 1;return i;}
    }
    return -1;
}

int main(int argc, char *argv[]){
    int myrank, size, len;
    flag_t flag;
    char* buffer;
    char processor[100];
    {
    //MPI TYPE    
    int count = 2;
    int array_of_blocklengths[] = { 1, 1 };
    MPI_Aint array_of_displacements[] = { offsetof( pair_t, value ),
                                          offsetof( pair_t, weight ) };
    MPI_Datatype array_of_types[] = { MPI_INT, MPI_INT };
    MPI_Datatype tmp_type, MPI_PAIR;
    MPI_Aint lb, extent;
    
    MPI_Type_create_struct( count, array_of_blocklengths, array_of_displacements,
                            array_of_types, &tmp_type );
    MPI_Type_get_extent( tmp_type, &lb, &extent );
    MPI_Type_create_resized( tmp_type, lb, extent, &MPI_PAIR );
    MPI_Type_commit( &MPI_PAIR );
    }

    int n, bag;
    printf("Enter number of items\n");
    scanf("%d", &n);
    printf("Enter bag size\n");
    scanf("%d",&bag_size);

    int sol[n], bestSol = INT_MIN, curSol = INT_MIN;
    int i;
    for (i = 0; i < n; i++) sol[i] = -1;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Get_processor_name(processor, &len);
    //Enter you code here

    
    if (myrank == 0) // master code
    {
        int i;
        printf("Enter %d value-weight pairs\n", n);
        pair_t * arr = (pair_t*)calloc(n, sizeof(pair-t));
        for (i = 0; i < n; i++)
            scanf("%d %d", &(arr[i].value), &(arr[i].weight);
        qsort(); // sort the pair according to value/weight ratio
        int nProc = argc;
        int busy[argc];
        memset(busy, '\0', argc*sizeof(char));
        busy[0] = 1;
        int idle = argc-1;
        MPI_Bcast(arr, n, MPI_PAIR, 0, MPI_COMM_WORLD)
        int fIdle;
        fIdle = firstIdle(busy, nProc, &idle);
        flag = PBM_TAG;
        MPI_Send(&flag, 1, MPI_INT, fIdle, MPI_ANY_TAG, MPI_COMM_WORLD);   
        MPI_Send(sol, n, MPI_PAIR, fIdle, MPI_ANY_TAG, MPI_COMM_WORLD); //Send initial sp 
        MPI_Send(bestSol, 1, MPI_INT, fIdle, MPI_ANY_TAG, MPI_COMM_WORLD);
        MPI_Status status;
        while (idle < nProc-1){
            MPI_Recv(&flag, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int src = status.MPI_SOURCE;
            if (flag == SOLVE_TAG){
                int arr[2];
                MPI_Recv(&bestSol, 1, MPI_INT, src, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(sol, n, MPI_INT, src, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            }
            if (flag == BnB_TAG) {
                int high; 
                MPI_Recv(&high, 1, MPI_INT, src, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                int assigned[2] = {};
                assigned[0] = assigned[1] = -1;
                if (high>bestSol){ //problem must be branched
                    assigned[0] = firstIdle(busy, n, &idle);
                    assigned[1] = firstIdle(busy, n, &idle);
                }
                MPI_Send(assigned, 2, MPI_INT, src, MPI_ANY_TAG, MPI_COMM_WORLD);
            }
            if (IDLE_TAG){ idle++; busy[src] = 1; }
        }
    }
    else if (myrank != 0) // slave code
    {
        int source, upper_bound;
        MPI_Status slaveStatus;
        int *currSol;
        pair_t * recvArr = (pair_t*)calloc(n, sizeof(pair_t));
        MPI_Bcast(recvArr,n,MPI_PAIR,0,MPI_COMM_WORLD);
        MPI_Recv(&flag,1,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD, MPI_ANY_TAG, MPI_COMM_WORLD, &slaveStatus);
        source=status.MPI_SOURCE;
        if (flag==END_TAG){ // receive the finishing message
            return;
        }
        if (PBM_TAG){ // receive the problem to be branched
            MPI_Recv(sol,n,MPI_INT,source,MPI_ANY_TAG,MPI_COMM_WORLD,&slaveStatus);
            MPI_Recv(&bestSol, 1, MPI_INT, source, MPI_ANY_TAG, MPI_COMM_WORLD, &slaveStatus);
            list_t list;
            list.head = NULL;
            list.len = 0;
            insert_into_list(&list, sol, n);
            while (!empty_list(&list)){
                int * arr = remove_from_list(&list);
                int high = upper_bound(arr);
                if (high > highSol){
                    int low = lower_bound(arr);
                    if (low > bestSol) {
                        bestSol = low;
                        currSol = arr;
                        int flag = SOLVE_TAG;
                        MPI_Send(&flag, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
                        MPI_Send(&bestSol, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
                        MPI_Send(currSol , n, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
                    }
                    if (high != low) {
                        int flag = BnB_TAG, num;
                        MPI_Send(&flag , 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
                        MPI_Send(&(list->len) , 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
                        MPI_Recv(&num, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COM_WORLD, &slaveStatus);
                        int arr[num];
                        MPI_Recv(arr, num, MPI_INT, 0, MPI_ANY_TAG, MPI_COM_WORLD, &slaveStatus);
                        branch(sol,n,  &list);
                        if (arr[i]

                    }

                }
            }



            

            //insert in queue
        
        
        
 bqueue.insert(auxSp); // insert the subproblem in the local queue
14 while(!bqueue.empty()) {
15 auxSp = bqueue.remove(); // pop a problem from the local queue
16 high = auxSp.upper_bound(pbm,auxSol); // upper bound
17 if ( high > bestSol ) {
18 low = auxSp.lower_bound(pbm,auxSol); // lower bound
19 if ( low > bestSol ) {
20 bestSol = low;
21 sol = auxSol;
22 outputPacket.send(MASTER, // send to the Master:            //MPI_Send(currSol,n,source,SOLVE_TAG,MPI_COMM_WORLD);
23 SOLVE_TAG, // the problem is solved
24 bestSol, // the best solution value
25 sol); // the solution vector
26 }        
    }
}

typedef node_t{
    int *arr; //length n always
    node_t * next;
} node_t;

typedef list_t {
    node_t * head;
    int len;
};

void insert_into_list(list_t * list, int *arr, int n){
    node_t * x = (node_t*) malloc(sizeof(node_t));
    x->arr = (int*)malloc(n*sizeof(int));
    int i;
    for (i=0;i<n;i++)
        (x->arr)[i] = arr[i];
    x->next = list->head;
    list->head = x;
    list->len += 1;
}

int * remove_from_list(list_t *list){
    if (list->len == 0) return NULL;
    node_t * temp = list->head;
    list->head = temp->next;
    list->len -= 1;
    int *arr = temp->arr;
    free(temp);
    return arr;
}

int empty_list(list_t * list){
    if (list->len == 0) return 1;
    return 0;
}
