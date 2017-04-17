#include <stdlib.h>

typedef struct node_t{
    int *arr; //length n always
    struct node_t * next;
} node_t;

typedef struct list_t {
    node_t * head;
    int len;
} list_t;

void insert_into_list(list_t * list, int *arr, int n);

int * remove_from_list(list_t *list);

int empty_list(list_t * list);
