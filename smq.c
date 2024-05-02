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

#include "smq.h"

/*
** tv2dbl()
**
** Convert a struct timeval into a floating point(double) representing
** the time.
*/
static double tv2dbl(struct timeval *tv) {
    return (double)tv->tv_sec + ((double)tv->tv_usec / (double)1000000.0);
}

/*
** gettime_dbl()
**
** Return the current time as a floating point (double) value instead
** of as a struct timeval *
*/
static double gettime_dbl(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return tv2dbl(&tv);
}

#ifdef INCLUDE_DB2TV
/*
** dbl2tv()
**
** Convert a floating point (double) value into a struct timeval where
** tv_sec and tv_usec are populated for seconds and microseconds.
*/
static int dbl2tv(struct timeval *tv, double t) {
    tv->tv_sec = (time_t)t;
    tv->tv_usec = (suseconds_t) ((t - (double)tv->tv_sec) * 1000000);
    return 0;
}
#endif

/*
** dbl2timespec()
**
** Convert a floating point (double) value into a struct timespec, used
** for abstime in pthread_cond_timedwait(). Important to note here we use
** nanoseconds instead of microseconds.
*/
static int dbl2timespec(struct timespec *spec, double t) {
    spec->tv_sec = (time_t) t;
    spec->tv_nsec = ((t - (double)spec->tv_sec) * 1000000000);
    return 0;
}

/*
** _smq_timeout_time()
**
** Given a timeout in milliseconds, get the current time and add to it
** the milliseconds in the future for the exiration time; then, convert
** this to struct timespec. This is to be used directly in pthread_cond_timedwait().
*/
static int _smq_timeout_time(struct timespec *abstime, int timeout_ms) {
    double current_time, expire_time;

    /* fetch the current time as a double */
    current_time = gettime_dbl();

    /* calculate the expiration time, which is in the future */
    expire_time = current_time + ((double)timeout_ms / (double)1000.0);

    return dbl2timespec(abstime, expire_time);
}

/*
** _smq_lock()
**
** Provide a lock
*/
static int _smq_lock(SMQ q) {
    return pthread_mutex_lock(&q->_tdata.lock);
}

/*
** _smq_unlock()
**
** Remove the lock
*/
static int _smq_unlock(SMQ q) {
    return pthread_mutex_unlock(&q->_tdata.lock);
}

/*
** _smq_cond_wait()
**
** Wait for a change in the condition variable as prescribed. The
** ``sig'' argument is SMQ_SIG_READ or SMQ_SIG_WRITE depending on
** which of the two variables. If abstime is NULL, then we wait
** forever for notification; otherwise, the abstime specifies the time
** of expiration.
*/
static int _smq_cond_wait(SMQ q, int sig, struct timespec *abstime) {
    /*
    ** If not NULL, expire at a specific time in the future (what we were
    ** provided.
    */
    if (abstime) {
        if (sig == SMQ_SIG_READ)
            return pthread_cond_timedwait(&q->_tdata.condr, &q->_tdata.lock, abstime);
        else if (sig == SMQ_SIG_WRITE)
            return pthread_cond_timedwait(&q->_tdata.condw, &q->_tdata.lock, abstime);
    } 
    /*
    ** If abstime is NULL, then wait an unspecified amount of time, which
    ** could be forever.
    */
    else {
        if (sig == SMQ_SIG_READ)
            return pthread_cond_wait(&q->_tdata.condr, &q->_tdata.lock);
        else if (sig == SMQ_SIG_WRITE)
            return pthread_cond_wait(&q->_tdata.condw, &q->_tdata.lock);
    }
    return -1;
}

/*
** _smq_wait_for_write()
**
** When the queue is full, this will wait until there is an
** availability to write, or, if a timeout happens, return -1
** back to the link indicating we were unable to write. The caller
** does the locking; this does the checking.
*/
static int _smq_wait_for_write(SMQ q, int ms) {
    struct timespec abstime, *_abstime = NULL;
    int value;

    /*
    ** Testing is unneeded as we do not limited the count
    */
    if (q->max_count <= 0)
        return 0;

    /*
    ** If ms is greater than 0, calculate the expiration time
    ** for the future.
    */
    if (ms > 0) {
        _smq_timeout_time(&abstime, ms);
        _abstime = &abstime;
    }

    for (; /* break inside */; ) {

        /*
        ** The condition is not yet met. The queue must be less than
        ** the max_count for us to be able to continue.
        */
        if (q->count >= q->max_count) {
            /*
            ** If wait time was 0, then we may have failed already. Otherwise, attempt
            ** to run our conditional wait, which may be time based or forever (depending
            ** on whether _abstime is NULL.
            */
            if (ms == 0 || ((value = _smq_cond_wait(q, SMQ_SIG_WRITE, _abstime))))
                return -1;
            continue;
        } 
        break;
    }

    return 0;
}

/*
** _smq_signal()
**
** Send a pthread_cond_signal to the appropriate variable for 
** reading or writing so the thread may continue.
*/
static int _smq_signal(SMQ q, int signal_type) {
    int retval = -1;
    switch (signal_type) {
        /*
        ** Notify a reader that there may be data available
        */
        case SMQ_SIG_READ:
            pthread_cond_signal(&q->_tdata.condr);
            retval = 0;
            break;
        /*
        ** Notify writer that the count may be lower than
        ** max_count now and it may be able to write new
        ** data.
        */
        case SMQ_SIG_WRITE:
            pthread_cond_signal(&q->_tdata.condw);
            retval = 0;
            break;
        default:
            break;
    }
    return retval;
}

/*
** _smq_link()
**
** Accept an SMQItem and add to the queue waiting up to ``ms'' time for
** the writability to become available.
*/
static int _smq_link(SMQ q, SMQItem item, int ms) {
    /* lock the mutex */
    _smq_lock(q);

    /*
    ** Wait for the notification that we can write more data
    ** to the queue. If we get non-zero (-1) then there was
    ** an error and we should error out ourselves.
    */
    if ((_smq_wait_for_write(q, ms)) < 0) {
        _smq_unlock(q);
        return -1;
    }

    /* If head is not defined, then set head and tail to the item */
    if (!q->head) {
        q->head = q->tail = item;
    /*
    ** otherwise, add to the tail
    */
    } else {
        q->tail->next = item;
        q->tail = q->tail->next;
    }

    /* increase count for number of items in queue */
    q->count++;

    /* signal listening reader about a change/update to the queue */
    _smq_signal(q, SMQ_SIG_READ);

    /* unlock the mutex */
    _smq_unlock(q);
    return 0;
}

/*
** _smq_wipe()
**
** Wipes/Removes all elements/items from the queue. Does not
** lock. Do not use directly. See smq_wipe() for the thread-safe
** version which should be used.
*/
static void _smq_wipe(SMQ q) {
    SMQItem item;

    while ((item = q->head)) {
        if (!(q->head = q->head->next))
            q->head = q->tail = NULL;
        item->next = NULL;
        q->count--;
        if (q->onfree)
            q->onfree (&item->msg[0]);
        free (item);
    }
}


/*
************************************************************************
**
** Standard API functions start here
**
************************************************************************
*/


/*
** smq_create()
**
** Create a simple message queue which can have messages sent to 
** and may be read from. The SMQ is thread-safe and offers an
** event-based versus polling approach for "data ready" minimizing
** CPU usage.
**
** @len: The length (or size) of all objects (items/elements) written
**  into the queue.
** @max_count: The maximum number of items allowed in the queue before
**  sends will begin to either (1) block; or (2) return with error. A
**  value of <= 0 indicates no limit to the queue and an indeterminate
**  size will be permitted.
** @onfree: Pointer to function to be called when freeing individual
**  items from the queue. This is typically only done on a wipe or
**  destroy operations.
*/
SMQ smq_create(int len, int max_count, void (*onfree)(void *)) {
    SMQ q;

    if (!(q = calloc(1, sizeof(*q))))
        return NULL;
    q->len = len;
    q->count = 0;
    q->onfree = onfree;
    q->max_count = max_count;
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->_tdata.lock, NULL);
    pthread_cond_init(&q->_tdata.condr, NULL);
    pthread_cond_init(&q->_tdata.condw, NULL);
    return q;
}

/*
** smq_send()
**
** Send a message into the provided queue to be received by a 
** consumer later. If max_count is reached, then smq_send() may
** block to wait for a write, or may return an error depending
** on arguments provided.
**
** @q: The queue object to write the data to
** @data: A pointer to the data to copy as a message. The data
**  itself must be the length specified as ``len'' to smq_create().
**  If data is NULL, an error may be immediately returned.
** @wait_ms: The milliseconds to wait for the write to be successful.
**  If count has reached max_count; then:
**   wait_ms < 0: Wait indefinitely to be able to write
**   wait_ms == 0: Only attempt the write for a single cycle, otherwise, 
**    return an error.
**   wait_ms > 0: Wait the specified number of milliseconds (blocked) and
**    return error if a time out occurs. 
**
** Returns: 0 on success, < 0 on error.
*/
int smq_send(SMQ q, void *data, int wait_ms) {
    SMQItem item;

    /* data cannot be NULL */
    if (!data)
        return -1;

    if (!(item = calloc(1, sizeof(*item) + q->len)))
        return -1;
    
    /* be sure this is set to 0 */
    item->next = NULL;

    /* copy the data into the queue */
    memmove(item->msg, data, q->len);

    /* record the time the message was received */
    gettimeofday(&item->tv, NULL);

    /* link/add the item into the list and notify consumer(s) */
    if ((_smq_link(q, item, wait_ms))) {
        free (item);
        return -1;
    }

    return 0;
}


/*
** smq_recv()
** 
** Receive/Consume a message from the SMQ.
**
** @q: The SMQ object to receive/read the message from.
** @data: Where to write the data to. Must be at least the
**  size of ``len'' bytes specified from smq_create(). Unlike
**  smq_send(), data may be NULL. If data is NULL, the message
**  is discarded and onfree() [if assigned] will be called 
**  to free the underlying object.
** @tv: Copy the struct timeval of when the message was originally
**  requested to send. If NULL, the structure will not be copied.
** @timeout_ms: The time in milliseconds to wait to receive a
**  message from the queue. 
**  timeout_ms == 0: Attempt to read only once. Do not wait for
**   new data to become available.
**  timeout_ms < 0: Wait for an indefinite period of time for
**   new data to become available.
**  timeout_ms > 0: Wait the specified milliseconds for new
**   data to become available.
**
** Returns > 0 if new data is available and was written into data.
** Returns == 0 if data was not available during the specified
**  period of time.
*/
int smq_recv(SMQ q, void *data, struct timeval *tv, int timeout_ms) {
    SMQItem item;
    int retval = 0, value;
    struct timespec abstime, *_abstime = NULL;

    if (timeout_ms > 0) {
        _smq_timeout_time(&abstime, timeout_ms);
        _abstime = &abstime;
    }

    /* perform a lock operation */
    _smq_lock(q);

    /*
    ** Loop to wait for condition, we'll perform a break when
    ** the condition is met, namely there is data to get.
    */
    for (; /* break inside */ ;) {
        if ((item = q->head)) {
            /* advance the head */
            if (!(q->head = q->head->next)) 
                q->head = q->tail = NULL;
            
            /* copy the send time if requested */
            if (tv)
                memcpy(tv, &item->tv, sizeof(*tv));

            /* copy the data */
            if (data)
                memmove(data, item->msg, q->len);
            else if (!data && q->onfree)
                q->onfree(&item->msg[0]);

            /* return value is > 0 indicating something exists */
            retval = 1;

            /* reduce count of elements */
            q->count--;

            /* free memory for the item */
            free (item);

            /* Notify potential writers which could be blocked */
            _smq_signal(q, SMQ_SIG_WRITE);

            /* leave the loop */
            break;
        }

        /*
        ** If timeout_ms is 0 then we have tried our 1 attempt. Otherwise, try
        ** to wait for a change in the read variable status or wait forever,
        ** depending on whether _abstime is NULL or not. If timeout_ms was
        ** less than 0, we tend to wait forever; otherwise, we wait up to those
        ** milliseconds.
        */
        if (timeout_ms == 0 || ((value = _smq_cond_wait(q, SMQ_SIG_READ, _abstime))))
            break;
    }

    /* unlock */
    _smq_unlock(q);

    return (retval);
}

/*
** smq_wipe()
**
** Does not destroy the queue object itself. However, removes all
** entries/items from the queue list.
**
** @q: The SMQ object where to remove all entries from.
**
*/
void smq_wipe(SMQ q) {
    /* Need a lock on the queue */
    _smq_lock(q);

    /* Call the non-locking function to remove entries */
    _smq_wipe(q);

    /* unlock */
    _smq_unlock(q);
}

/*
** smq_get_count()
**
** Return the number of items/elements currently in the
** queue.
**
** @q: The SMQ object to get the count from.
**
** Returns number of items in the queue.
*/
int smq_get_count(SMQ q) {
    int count;

    /* get a lock */
    _smq_lock(q);

    /* store the count */
    count = q->count;

    /* unlock */
    _smq_unlock(q);

    /* return count */
    return count;
}

/*
** smq_destroy()
**
** Destroy and free a queue. All items still in the queue will be 
** wiped and the actual object itself will be freed.
**
** @q: The SMQ object to destroy
*/
int smq_destroy(SMQ q) {
    /* perform a lock */
    _smq_lock(q);

    /* remove any items still in the queue */
    _smq_wipe(q);
    pthread_cond_destroy(&q->_tdata.condr);
    pthread_cond_destroy(&q->_tdata.condw);
    _smq_unlock(q);
    pthread_mutex_destroy(&q->_tdata.lock);

    free (q);

    return 0;
}

