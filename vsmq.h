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

#ifndef __VSMQ_H__
#define __VSMQ_H__

typedef SMQ vSMQ;

typedef struct st_vsmpq {
    int sz;
    void *ptr;
} vSMQ_WRAP;


extern vSMQ vsmq_create(int);
extern int vsmq_send(vSMQ q, void *, int, int);
extern void *vsmq_recv(vSMQ, int *, int);
extern int vsmq_destroy(vSMQ);
extern int vsmq_get_count(vSMQ);
extern void vsmq_wipe(vSMQ);


#endif /* __VSMQ_H__ */