/*
** This is free and unencumbered software released into the public domain.
**
** Anyone is free to copy, modify, publish, use, compile, sell, or
** distribute this software, either in source code form or as a compiled
** binary, for any purpose, commercial or non-commercial, and by any
** means.
**
** In jurisdictions that recognize copyright laws, the author or authors
** of this software dedicate any and all copyright interest in the
** software to the public domain. We make this dedication for the benefit
** of the public at large and to the detriment of our heirs and
** successors. We intend this dedication to be an overt act of
** relinquishment in perpetuity of all present and future rights to this
** software under copyright law.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
** OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
** OTHER DEALINGS IN THE SOFTWARE.
*/
/*
** Original Author: Keith Fralick
*/

#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <pthread.h> 
#include <unistd.h> 
#include <sys/time.h>

#ifndef __SMQ_H__
#define __SMQ_H__

#define SMQ_SIG_READ    1
#define SMQ_SIG_WRITE   2

typedef struct st_simple_queue_item {
    struct st_simple_queue_item *next;
    struct timeval tv;
    char msg[1];
} *SMQItem;

typedef struct st_simple_queue {
    /*
    ** The length of each member in the queue
    */
    int len;

    /*
    ** The number of elements currently in the queue
    */
    int count;

    /* 
    ** The maximum number of entries in the queue
    ** before we wait. A value less than 1 indicates
    ** we are not limiting queue entries.
    */
    int max_count;

    void (*onfree)(void *);

    SMQItem head, tail;
    struct {
        pthread_mutex_t lock;

        /*
        ** Condition variable for reading (reading ready)
        */
        pthread_cond_t condr;

        /*
        * Condition variable for writing (writing ready)
        */
        pthread_cond_t condw;

    } _tdata;
} *SMQ;


extern SMQ smq_create(int, int, void (*)(void *));
extern int smq_send(SMQ, void *, int);
extern int smq_recv(SMQ, void *, struct timeval *, int);
extern int smq_get_count(SMQ);
extern int smq_destroy(SMQ);
extern void smq_wipe(SMQ);


#endif /* __SMQ_H__ */