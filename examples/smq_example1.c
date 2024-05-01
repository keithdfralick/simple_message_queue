/*
** This is free and unencumbered software released into the public domain.
**
** Refer to LICENSE for additional information.
*/

/*
** This example uses only SMQ to create a list of numbers from 0 to 
** some value and reads it.
**
** Compile: gcc smq.c smq_example1.c -pthread -o smq_example1
*/
#include "smq.h"

#define MAXVALUE    100
#define THREADS 8

typedef struct st_iter {
    int value;
} ITER;

void *myreader(void *_q) {
    SMQ q = _q;
    ITER iter;

    /* Read and print values */
    while ((smq_recv(q, &iter, NULL, 100))) 
        printf("ID gotten: %d\n", iter.value);

    return NULL;
}

int main(void) {
    SMQ q;
    ITER iter;
    pthread_t tids[THREADS];
    int i;

    /* Attempt to create a queue */
    if (!(q = smq_create(sizeof(iter), 2, NULL))) {
        fprintf(stderr, "Cannot create SMQ\n");
        return -1;
    }

    /* kick off the reader threads */
    for (i = 0; i < THREADS; i++) 
        pthread_create(&tids[i], NULL, myreader, q);

    /* push out values in our little structure */
    for (i = 0; i <= MAXVALUE; i++) {
        iter.value = i;

        smq_send(q, &iter, 100);
    }

    /* wait for threads to exit */
    for (i = 0; i < THREADS; i++)
        pthread_join(tids[i], NULL);

    /* destroy queue object */
    smq_destroy(q);

    return 0;
}