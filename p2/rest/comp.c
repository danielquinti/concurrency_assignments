#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include "compress.h"
#include "chunk_archive.h"
#include "queue.h"
#include "options.h"

#define CHUNK_SIZE (1024*1024)
#define QUEUE_SIZE 100

#define COMPRESS 1
#define DECOMPRESS 0

struct args {
    queue in;
    queue out;
    chunk (*process)(chunk);
};

// take chunks from queue in, run them through process (compress or decompress), send them to queue out
void *thread_worker(void *vargp){
    chunk ch, res;
    struct args *args= vargp;
    while(q_elements(args->in)>0) {
        ch= q_remove(args->in);
        res = args->process(ch);
        //---------
        res->num=ch->num;   //el número de chunk se lo pasamos al res
        //---------
        free_chunk(ch);
        q_insert(args->out, res);
    }
}

// Compress file taking chunks of opt.size from the input file,
// inserting them into the in queue, running them using a worker,
// and sending the output from the out queue into the archive file
void comp(struct options opt) {
    int fd, chunks, i;
    struct stat st;
    char comp_file[256];
    archive ar;
    chunk ch;
    pthread_t *threads;
    struct args args;
    printf("Comprimiendo...\n");

    if((threads=malloc(sizeof(pthread_t)*opt.num_threads))==NULL) {//Creamos un array de threads
        printf("Out of memory\n");
        exit(1);
    }

    args.process=zcompress;  //Incializamos args.process con la función a ajecutar

    if((fd=open(opt.file, O_RDONLY))==-1) {
        printf("Could not open %s\n", opt.file);
        exit(0);
    }
    
    fstat(fd, &st);
    chunks = st.st_size/opt.size+(st.st_size % opt.size ? 1:0);

    strncpy(comp_file, opt.file, 255);
    strncat(comp_file, ".ch", 255);
    
    ar = create_archive_file(comp_file);

    args.in  = q_create(QUEUE_SIZE);
    args.out = q_create(QUEUE_SIZE);

    // read input file and send chunks to the in queue
    for(i=0; i<chunks; i++) {
        ch = alloc_chunk(opt.size);
        //---------
        ch->num=i;      //le damos un valor numérico al chunk
        //---------
        ch->size = read(fd, ch->data, opt.size);
        q_insert(args.in, ch);
    }

    //Creación de los diferentes threads que van a ejecutar thread_worker
    for(i=0; i<opt.num_threads; i++){

        if ( 0 != pthread_create(&threads[i],NULL,thread_worker, &args)){
            printf("Could not create thread #%d", i);
            exit(1);
        } 
    }

    //esperamos a que acaben de hacer su trabajo
    printf("\r0.0%%");
    for (i = 0; i <opt.num_threads; i++){
        fflush( stdout );
        pthread_join(threads[i], NULL);
        printf("\r%.1f%%",(((float)i+1)/(float)opt.num_threads)*100);
        

    }
    printf("\n");
    
    // send chunks to the output archive file
    for(i=0; i<chunks; i++) {
        ch = q_remove(args.out);
        //---------
        add_chunk(ar, ch); //añadimos el chunk con su número correspondiente
        //---------
        free_chunk(ch);
    }

    free(threads);
    close_archive_file(ar);
    close(fd);
    q_destroy(args.in);
    q_destroy(args.out);
}


// Decompress file taking chunks of opt.size from the input file,
// inserting them into the in queue, running them using a worker,
// and sending the output from the out queue into the decompressed file

void decomp(struct options opt) {
    int fd, i;
    struct stat st;
    char uncomp_file[256];
    archive ar;
    chunk ch;
    pthread_t *threads;
    struct args args;
    chunk *ch_array;

    
    printf("Descomprimiendo...\n");

    if((ar=open_archive_file(opt.file))==NULL) {
        printf("Could not open archive file\n");
        exit(0);
    };
    
    strncpy(uncomp_file, opt.file, strlen(opt.file) -3);
    uncomp_file[strlen(opt.file)-3] = '\0';

    if((fd=open(uncomp_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))== -1) {
        printf("Could not create %s: %s\n", uncomp_file, strerror(errno));
        exit(0);
    } 

    //Hacemos los mallocs correspondientes
    if((threads=malloc(sizeof(pthread_t)*opt.num_threads))==NULL) {//Creamos un array de threads
        printf("Out of memory\n");
        exit(1);
    }
    if((ch_array=malloc(sizeof(chunk)*chunks(ar)))==NULL) { //incializamos el array de chunks
        printf("Out of memory\n");
        exit(1);
    }
    //------------------------------------

    //incializamos args
    args.in  =q_create(QUEUE_SIZE);
    args.out =q_create(QUEUE_SIZE);
    args.process=zdecompress;  //Incializamos args.process con la función a ajecutar

    //read chunks with compressed data
    for(i=0; i<chunks(ar); i++) {
        ch = get_chunk(ar, i);
        //---------
        ch->num=i;
        //---------
        q_insert(args.in, ch);
    }

    //Creación de los diferentes threads que van a ejecutar thread_worker
    for(i=0; i<opt.num_threads; i++){

        if ( 0 != pthread_create(&threads[i],NULL,thread_worker, &args)){
            printf("Could not create thread #%d", i);
            exit(1);
        } 
    }

    //esperamos a que acab de hacer su trabajo
    printf("\r0.0%%");
    for (i = 0; i <opt.num_threads; i++){
        fflush( stdout );
        pthread_join(threads[i], NULL);
        printf("\r%.1f%%",(((float)i+1)/(float)opt.num_threads)*100);
    }
    printf("\n");
    // write chunks from output to decompressed file
    for(i=0; i<chunks(ar); i++) {
        ch=q_remove(args.out);
        //---------
        ch_array[ch->num]=ch;   //vamos añadiendo los chunks en un array ordenado
        //---------
    }
    
    for(i=0; i<chunks(ar); i++) {
        //---------
        ch=ch_array[i];
        //---------
        write(fd, ch->data, ch->size);
        free_chunk(ch);
    }   
    free(threads);
    free(ch_array);
    close_archive_file(ar);    
    close(fd);
    q_destroy(args.in);
    q_destroy(args.out);
}

int main(int argc, char *argv[]) {    
    struct options opt;
    
    opt.compress=COMPRESS;
    opt.num_threads=3;
    opt.size=CHUNK_SIZE;
    opt.queue_size=QUEUE_SIZE;
    
    read_options(argc, argv, &opt);
    
    if(opt.compress==COMPRESS) comp(opt);
    else decomp(opt);
}
    
    
