# Simple Message Queue (SMQ / vSMQ)

Simple Message Queue (SMQ / VSMQ) provides a simple set of routines 
(written in C) which provide a thread-safe mechanism for sending 
messages between threads. Only 6 public (non-static) functions are
exposed and required to send the messages. Locking is performed
by the codebase itself. Threads waiting on messages have timeout
values (in milliseconds) and condition variables are used to limit
CPU usage.

## SMQ vs vSMQ

SMQ is provides for a fixed size of data to be written as a message,
where-as vSMQ provides an optional extension, allowing memory 
pointers to be used and passed around more easily from the message 
queue. Essentially, SMQ is the base code and vSMQ is a wrapper
providing more flexible access. 

## Examples

Examples to both SMQ and vSMQ usage is available in the examples/
directory. Please read the comments at the top of each file for
information on how to compile the example.

### examples/smq_example1.c

Creates different objects containing a value between 0 and 100 and
writes those into the queue. Threads then read and simply output
the value. 

### examples/smq_example2.c

Uses a pointer and stores it and shows how to use ``onfree'' callback
to free memory. Has no threading as part of the example; provides
some additional insight into how the information is moved/stored.

### examples/vsmq_example1.c

Pushes words from /usr/share/dict/words into a queue and is read
by thread processes. Uses the vSMQ wrappers to simply write a string
into the queue. Additional comments are in the file.ß

### Public Functions (SMQ)

`SMQ smq_create(int data_size, int max_queue_size, void (*onfree_callback)(void *))`

Creates an SMQ create object. 

* The ``data_size`` is the size of each element written and read from the queue. A
data_size of less than or equal to 0 will result in NULL being returned.
* The ``max_queue_size'' is the maximum number of items which can exist in the queue.
Once this value is reached, then smq_send will block (wait state) for the queue's
readers to remove items, until the queue size is below ``max_queue_size''. 
If ``max_queue_size'' is <= 0, then the queue will be virtually unlimited (except
by memory).
* The ``onfree_callback'' is a function which is called on destroy or wipe for any 
item should additional memory need to be freed or addressed in some way. If NULL
is specified, no callback is performed.

Returns the SMQ object if successful, otherwise NULL is returned.


`int smq_send(SMQ smq, void *data, int timeout_ms)`

Sends a message to the queue. 
* smq is the object previously created with smq_create and is the queue. 
* Data is the data to copy into the queue and must be the size
specified by the ``data_size'' argument to smq_create. 
* Timeout_ms is the amount of time (in milliseconds) to wait for a write. 

* If data is NULL, nothing will be written and error will be returned.
* If timeout_ms is < 0, then wait will be unspecified (indefinite); if 
timeout_ms is 0, then we will not wait for a write-ready state, we attempt
a write only once cycle before failure. If timeout_ms is greater than 0, then
we wait that specified amount of time in milliseconds for data to write-ready.

Returns 0 on success and < 0 on failure (unable to write).

`int smq_recv(SMQ smq, void *data, struct timeval *tv, int tmimeout_ms)`

`int smq_get_count(SMQ smq)`
`int smq_destroy(SMQ smq)`
`void smq_wipe(SMQ smq)`

