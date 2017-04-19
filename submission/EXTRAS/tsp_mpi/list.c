#include <stdlib.h>
#include "list_header.h"


void insert_into_list(list_t * list, int *arr, int n){
    node_t * x = (node_t*) malloc(sizeof(node_t));
    x->arr = arr;
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
