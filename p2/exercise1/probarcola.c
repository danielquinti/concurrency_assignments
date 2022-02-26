#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "options.h"
#include "queue.h"


struct thread_info {
    pthread_t       thread_id;        // id returned by pthread_create()
    int             thread_num;       // application defined thread #
};

struct args {
    int 		thread_num;       // application defined thread #
    int 	        delay;			  // delay between operations
    int		iterations;
    int 	*counter;
    queue 	    *q;
    pthread_mutex_t *mutex;			  // Mutex for the array
};

void *swap1(void *ptr)
{
    struct args *args =  ptr;
    pthread_mutex_lock(args->mutex);
    int *copy2 = malloc(sizeof(int));
    (*copy2)=(*args->counter);
    printf("Inserting %d\n",*copy2);
    printf("Adress: %p\n",copy2);
    q_insert(*(args->q),copy2);
    //printQueue(*(args->q));
    (*args->counter)++;
    pthread_mutex_unlock(args->mutex);
    return NULL;
}

void *swap2(void *ptr)
{
    struct args *args =  ptr;
    pthread_mutex_lock(args->mutex);
    int *copy=malloc(sizeof (int));
    copy=q_remove(*(args->q));
    printf("Removing %d\n",*copy);
    pthread_mutex_unlock(args->mutex);
    free(copy);
    return NULL;
}


void start_threads(struct options opt)
{
    int i;
    struct thread_info *threads;
    struct args *args;

    srand(time(NULL));

    queue *q = malloc(sizeof(queue));
    *q=q_create(100);

    printf("creating %d threads\n", opt.num_threads);
    threads = malloc(sizeof(struct thread_info) * opt.num_threads);
    args = malloc(sizeof(struct args) * opt.num_threads);

    if (threads == NULL || args==NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    int *counter = malloc(sizeof(int));
    (*counter)=0;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    // Create num_thread threads running swap()
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].thread_num = i;
        args[i].thread_num = i;
	args[i].counter    = counter;
	args[i].q	   = q;
        args[i].delay      = opt.delay;
        args[i].iterations = opt.iterations;
        args[i].mutex=&mutex; // Assign the same mutex to every thread

        if ( 0 != pthread_create(&threads[i].thread_id, NULL,
                                 swap1, &args[i])) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].thread_num = i;
        args[i].thread_num = i;
	args[i].counter    = counter;
	args[i].q	   = q;
        args[i].delay      = opt.delay;
        args[i].iterations = opt.iterations;
        args[i].mutex=&mutex; // Assign the same mutex to every thread

        if ( 0 != pthread_create(&threads[i].thread_id, NULL,
                                 swap2, &args[i])) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    // Wait for the threads to finish
    for(i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].thread_id, NULL);

    free(args);
    free(threads);
    free(counter);
    q_destroy(*q);
    pthread_exit(NULL);
}

int main (int argc, char **argv)
{
    struct options opt;

    // Default values for the options
    opt.num_threads = 10;
    opt.buffer_size = 10;
    opt.iterations  = 100;
    opt.delay       = 10;

    read_options(argc, argv, &opt);

    start_threads(opt);

    exit (0);
}
