#include<omp.h>
#include<stdio.h>
#include <stdlib.h>

int max(int a, int b) { return (a > b)? a : b; }
typedef struct pair_t{
    int value;
    int weight;
} pair_t;

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

int knapSack(int W, int wt[], int val[], int n) {
   if (n == 0 || W == 0)
       return 0;
   if (wt[n-1] > W)
       return knapSack(W, wt, val, n-1);
   else {
        int a=0,b=0;
#pragma omp parallel sections 
        {
            #pragma omp section 
            {
                a = val[n-1] + knapSack(W-wt[n-1], wt, val, n-1);	
            }
            #pragma omp section
            {
                b = knapSack(W, wt, val, n-1);
            }
        }
        if(a>b)
            return a;
        else
            return b;
    }
}
 
 
int main()
{
    omp_set_num_threads(4);
    int  W;
    int i=0;
    int n;
    scanf("%d %d",&n, &W);
    int *wt = (int*)malloc(n*sizeof(int));
    int *val = (int*)malloc(n*sizeof(int));
    for (i=0;i<n;i++)
        scanf("%d %d",&val[i],&wt[i]);
    pair_t *inp = (pair_t*)malloc(n*sizeof(pair_t));
    for(i=0;i<n;i++){
    	inp[i].value=val[i];
    	inp[i].weight=wt[i];
    }	
    qsort(inp, n, sizeof(pair_t), compar);
    for(i=0;i<n;i++){
        val[i]=inp[i].value;
        wt[i]=inp[i].weight;
    }
    printf("Optimal value that can be in knapsack = %d\n", knapSack(W, wt, val, n));
    return 0;
}
