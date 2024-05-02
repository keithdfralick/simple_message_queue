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

#include "vsmq.h"

/*
** _vsmq_free()
**
** Executed as a part of onfree for the queue to release memory in the
** event of wipe or destroy.
*/
static void _vsmq_free(void *p) {
    vSMQ_WRAP *wrap = p;

    if (wrap->ptr)
        free (wrap->ptr);
}

/*
** vsmq_create()
**
** Wrapper for smq_create(). Returns pointer to the vSMQ (alias of SMQ).
**
** @queue_size: The maximum size of the queue before blocking/waiting
**  occurs.
*/
vSMQ vsmq_create(int queue_size) {
    return smq_create(sizeof(vSMQ_WRAP), queue_size, _vsmq_free);
}

/*
** vsmq_send()
**
** Wrapps smq_send, allowing for a variable sized dynamic object to be 
** inserted.
**
** @q: The vSMQ object
** @data: The data to insert, which needs to be freed later.
** @sz: The size of the data.
** @timeout_ms: The timeout period to write for write.
**
** Returns 0 on success or -1 on error.
*/
int vsmq_send(vSMQ q, void *data, int sz, int timeout_ms) {
    vSMQ_WRAP wrap;
    void *p;
    int retval;

    /* data must be ``something'' and size must also be something, too */
    if (!data || sz <= 0)
        return -1;

    /* allocate memory for the message */
    if (!(p = calloc(1, sz + 1)))
        return -1;

    /* copy over the data */
    memcpy(p, data, sz);

    /* record the size and point to the copied data ptr */
    wrap.sz = sz;
    wrap.ptr = p;

    /* send to the queue */
    if ((retval = smq_send(q, &wrap, timeout_ms)) < 0) {
        /* error, free the memory and return -1 */
        free (p);

        return -1;
    }
    return retval;
}

/*
** vsmq_recv()
** 
** Wrapper which receives a message, but instead of passing back
** an integer, we pass back the memory region which was allocated
** previously and copied. The receiver must free() this memory.
**
** @q: The vSMQ object.
** @sz: Pointer to integer, which will be written the size of the 
**  data pointed to. If NULL, the size will not be written and the
**  pointer will still be returned, if available.
** @timeout_ms: The timeout to wait for new data to become available.
**
** Returns pointer to the data copied to the queue previously.
*/
void *vsmq_recv(vSMQ q, int *sz, int timeout_ms) {
    vSMQ_WRAP wrap;

    /* No data available */
    if (!(smq_recv(q, &wrap, NULL, timeout_ms)))
        return NULL;
    if (sz)
        *sz = wrap.sz;
    return wrap.ptr;
}

/*
** vsmq_destroy()
**
** Wrapper for smq_destroy()
*/
int vsmq_destroy(vSMQ q) {
    return smq_destroy(q);
}

/*
** vsmq_get_count()
**
** Wrapper for smq_get_count()
*/
int vsmq_get_count(vSMQ q) {
    return smq_get_count(q);
}

/*
** vsmq_wipe()
**
** Wrapper for smq_wipe()
*/
void vsmq_wipe(vSMQ q) {
    smq_wipe(q);
}
