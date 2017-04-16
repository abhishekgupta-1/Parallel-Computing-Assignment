#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int n;
int **graph;

int **alloc_2d_int(int rows, int cols) {
    int *data = (int *)malloc(rows*cols*sizeof(int));
    int **array= (int **)malloc(rows*sizeof(int*));
    for (int i=0; i<rows; i++)
        array[i] = &(data[cols*i]);
    return array;
}

int minKey(int key[], int mstSet[]) {
   int min = INT_MAX, min_index = 0;
   for (int v = 0; v < n; v++)
     if (mstSet[v] == 0 && key[v] < min)
         min = key[v], min_index = v;
   return min_index;
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
    int sum=auxSp[n],i=0;
    for(i=0;i<n;i++){
        if(auxSp[i]==-1){
            sum = sum + min(graph[i],i);
        }
    }
    return sum;
}

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
    if (inside == 0) {
        key[0] = 0;
        for (j=0;j<n;j++)
            if (0!=j && key[j] > graph[0][j])
                key[j] = graph[0][j];
    }
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
int main(){

    int i,j;
    printf("Enter number of nodes\n");
    scanf("%d",&n);
    graph = alloc_2d_int(n,n);
    printf("Enter adjacency matrix\n");
    for (i=0;i<n;i++)
        for (j=0;j<n;j++)
            scanf("%d", &graph[i][j]);

    int *initial_sP = (int*)malloc((n+4)*sizeof(int));
    for (i=0;i<n;i++)
        initial_sP[i] = -1;
    initial_sP[n] = 00; //curCost
    initial_sP[n+1] = INT_MAX;
    initial_sP[n+2] = 0; //elem which has to take decision
    initial_sP[n+3] = 0;  //flag to check if to to dfs
    printf("%d\n",upper_bound(initial_sP));
    printf("%d",lower_bound(initial_sP));
    return 0;
}
