#include "mvm.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_THREADS 10

#define MEM_SIZE (2 << 21)

void *function(void *whoami) {
    int i;
    int *p;
    long me = (long)whoami;
    char *aux;

    INSTRUMENT;
    printf("thread %ld active\n", me);

    p = (int *)mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    INSTRUMENT;
    if (!p) {
        INSTRUMENT;
        printf("(%ld) mmap error - returning\n", me);
        fflush(stdout);
        return NULL;
    }

    INSTRUMENT;
    for (i = 0; i < 10; i++) {
        p[i] = i;
    }

    INSTRUMENT;
    for (i = 0; i < 10; i++) {
        INSTRUMENT;
        printf("(%ld) p[%d] = %d\n", me, i, p[i]);
        fflush(stdout);
    }
    aux = (char *)p;
    INSTRUMENT;
    aux += MEM_SIZE >> 1;
    p = (int *)aux;
    INSTRUMENT;
    for (i = 0; i < 10; i++) {
        INSTRUMENT;
        printf("(%ld) p[%d] = %d\n", me, i, p[i]);
        fflush(stdout);
    }
}

int main(int argc, char *argv) {

    pthread_t tid[NUM_THREADS];
    int i = 0;

    goto job;

job:
    pthread_create(&tid[i], NULL, function, (void *)i);
    INSTRUMENT;
    if (++i < NUM_THREADS) {
        goto job;
    }

    INSTRUMENT;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(tid[i], NULL);
    }

    return 0;
}
