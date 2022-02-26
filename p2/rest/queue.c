#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// circular array
typedef struct _queue {
    int size;
    int used;
    int first;
    void **data;
    pthread_mutex_t mutex;
    pthread_cond_t empty;
    pthread_cond_t full;
} _queue;

#include "queue.h"

queue q_create(int size) {
    queue q = malloc(sizeof(_queue));
    
    q->size  = size;
    q->used  = 0;
    q->first = 0;
    q->data = malloc(size*sizeof(void *));
    pthread_mutex_init(&q->mutex,NULL);
    pthread_cond_init(&q->empty,NULL);;
    pthread_cond_init(&q->full,NULL);
    return q;
}

int q_elements(queue q) {
    int used;
    pthread_mutex_lock(&q->mutex);
    used=q->used;
    pthread_mutex_unlock(&q->mutex);
    return used;
    
}

int q_insert(queue q, void *elem) {

    pthread_mutex_lock(&q->mutex);
    while(q->size==q->used) pthread_cond_wait(&q->full,&q->mutex);
    
    q->data[(q->first+q->used) % q->size] = elem;    
    q->used++;
    if(q->used==1)
	pthread_cond_signal(&q->empty);
    pthread_mutex_unlock(&q->mutex);
    return 1;
}

void *q_remove(queue q) {
    void *res;
    pthread_mutex_lock(&q->mutex);
    while(q->used==0) pthread_cond_wait(&q->empty,&q->mutex);
    res=q->data[q->first];
    q->first=(q->first+1) % q->size;
    q->used--;
    if((q->used)==(q->size)-1)
	pthread_cond_signal(&q->full);
    pthread_mutex_unlock(&q->mutex);
    return res;
}

void q_destroy(queue q) {
    pthread_cond_destroy(&q->empty);
    pthread_cond_destroy(&q->full);
    pthread_mutex_destroy(&q->mutex);
    free(q->data);
    free(q);
}

void printQueue(queue q){
    int j=q->first;
    int i;
    printf(" ");
    for (i=0;i!=q->used; i++){
        printf("Contenido: %d|",*(int*)q->data[j]);
	printf("Direccion: %p|",(int*)q->data[j]);
	printf("Direccion celda: %p\n", &q->data[j]);
        j++;
    }
    if (i==0){
        printf("(Empty)");
    }
    printf("\n");
}
