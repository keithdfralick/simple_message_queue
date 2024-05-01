/*
** This is free and unencumbered software released into the public domain.
**
** Refer to LICENSE for additional information.
*/

/*
** This example shows how you may use a pointer and ``onfree'' to
** free any sub-objects on wipe/destroy. To keep this example
** simple, there are no threads involved.
**
** Compile: gcc smq.c smq_example2.c -o smq_example2
*/

#include "smq.h"

/*
** Our example structure which we'll use to allocate
** memory for using malloc().
*/
typedef struct st_example {
    int value;
} ExampleStruct;

/*
** Allocate memory for ExampleStruct and insert the
** number into value for argument ``n.'' Then, return
** a pointer to the structure.
*/
ExampleStruct *example_make(int n) {
    ExampleStruct *ex;

    if (!(ex = malloc(sizeof(*ex))))
        return NULL;
    ex->value = n;
    return ex;
}

/*
** Function showing how an allocated pointer may be freed
** on wipe or destroy.
*/
void example_free(void *ptr) {
    ExampleStruct *example;

    /* 
    ** Because the value was copied, the pointer exists as a
    ** pointer-to-pointer. There are two potential approaches
    ** here.
    **
    ** 1) Cast from void * to void ** and deference the pointer; or
    ** 2) Use memcpy/memmove to copy the data pointed to by ptr
    **  into the address of the pointer.
    **
    ** Either of the two methods above yield the same result.
    */
    
    /*
    ** Method #1
    */
    example = *(void **)ptr;

    /*
    ** Method #2
    */
    /* memcpy(&example, ptr, sizeof(example)); */

    /* print our function's name, the address of the pointer and the value within */
    printf("[%s] pointer: %p value=%d\n", 
        __FUNCTION__, example, example->value);

    /* free the memory */
    free (example);
}

int main(void) {
    SMQ q;
    ExampleStruct *ex;
    int i;

    /*
    ** Create a queue and use the size of the pointer to ExampleStruct only.
    ** On a 64-bit system, a sizeof(ExampleStruct *) is probably 8 bytes. If
    ** free is necessary, make a call to example_free().
    */
    if (!(q = smq_create(sizeof(ExampleStruct *), 100, example_free))) {
        fprintf(stderr, "Unable to create a queue\n");
        return -1;
    }

    /*
    ** Create 10 pointers and each one has a value of 0 through 9. Send
    ** those out into the queue.
    */
    for (i = 0; i < 10; i++) {
        /* create object populated with i. This has no error checking, but things usually should :) */
        ex = example_make(i);

        /* print out the pointer address */
        printf("original pointer to object [%d]: %p\n", i, ex);

        /* push to the queue as a pointer. Remember, this will be ``copied'' */
        smq_send(q, &ex, 10);
    }

    /* print number of items in queue */
    printf("\n-> Current Items in Queue: %d\n\n", smq_get_count(q));

    /*
    ** Read the first 5 entries and receive the data
    */
    for (i = 0; i < 5; i++) {
        ExampleStruct *ptr;

        /* receive from queue, since it is a pointer, remember to use & so it is copied */
        if ((smq_recv(q, &ptr, NULL, 10))) {
            /* print the pointer address and the value of the ExampleStruct->value */
            printf("smq_recv pointer: %p value=%d\n", 
                ptr, ptr->value);

            /* Free the pointer, since it was originally allocated with malloc() */
            free (ptr);
        }
    }

    puts("");

    /*
    ** The last 5 will be destroyed and the ``onfree'' function will be called to free
    ** the memory.
    */
    smq_destroy(q);

    return 0;
}