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

`SMQ smq_create(int max_queue_size, int data_size, void (*onfree_callback)(void *))`

Creates an SMQ create object. Returns NULL if unable to create the queue. 

`int smq_send(SMQ, void *, int)`
`int smq_recv(SMQ, void *, struct timeval *, int)`
`int smq_get_count(SMQ)`
`int smq_destroy(SMQ)`
`void smq_wipe(SMQ)`

