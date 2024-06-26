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

## Public Functions (SMQ)

The following are the 6 functions made available by this code base. 4 of the
6 would be commonly used with the other two less so. It would be uncommon
to use wipe or get_count, but these are available regardless.

<br><br>

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

<br><br>
`int smq_send(SMQ smq, void *data, int timeout_ms)`

Sends a message to the queue. 
* smq is the object previously created with smq_create and is the queue. 
* Data is the data to copy into the queue and must be the size
specified by the ``data_size'' argument to smq_create. If data is NULL, nothing will
be written and error will be returned.
* Timeout_ms is the amount of time (in milliseconds) to wait for a write. If
timeout_ms is < 0, then wait will be unspecified (indefinite); if 
timeout_ms is 0, then we will not wait for a write-ready state, we attempt
a write only once cycle before failure. If timeout_ms is greater than 0, then
we wait that specified amount of time in milliseconds for data to write-ready.

Returns 0 on success and < 0 on failure (unable to write).

<br><br>
`int smq_recv(SMQ smq, void *data, struct timeval *tv, int timeout_ms)`

Receives a message from the queue (consumer).
* smq is the object previously created with smq_create.
* data is the data to copy from the queue into the address pointed to by data, whose
size will be that defined from smq_create.
* tv will be copied and is the time the message was received (timestamp). If
tv is NULL, this will not result in error, but the timestamp will not be copied.
* timeout_ms is the number of milliseconds to wait for data to become available. 
If timeout_ms is < 0, then we will wait an indefinite amount of time. If timeout_ms
is 0, then we will attempt to read the data, but only once; we will not wait for
new data to become available. If timeout_ms is greater than 0, then we wait that
period of time (in milliseconds) for new data to become available.

Returns non-zero on receipt of data or 0 if no data is available.

<br><br>
`int smq_get_count(SMQ smq)`

Get the count of the number of items in the queue.
* smq is the object created earlier by smq_create.

Returns the number of items currently in the queue.

<br><br>
`int smq_destroy(SMQ smq)`

Destroy and free all items in the queue. The object is freed along with every item
being removed. Mutexes and Condition variables are also destroyed.
* smq is the object created earlier by smq_create

Returns 0 at the moment.

<br><br>
`void smq_wipe(SMQ smq)`

Removes all items currently in the queue, but the queue itself remains available.
* smq is the object created by smq_create

Returns nothing as-is void. All items are dropped/purged.

<br><br>
## Public Functions (vSMQ)

The functions provided by vSMQ are essentially the same as those provided by SMQ 
and vSMQ is essentially an extension/wrapper around SMQ which allows one to use
greater flexibility with their size payloads.

`vSMQ vsmq_create(int max_queue_size)`

Create a vSMQ object. The vSMQ object is an alias to SMQ, but with special settings
pre-defined. 

* The max_queue_size is the maximum size of the queue. Works as that from smq_create.

<br><br>
`int vsmq_send(vSMQ q, void *data, int dsize, int timeout_ms)`

Sends a message to the queue.

* q is the vSMQ created previously by vsmq_create.
* data is the data to write into the queue.
* dsize is the length of the data being written to the queue 
* timeout_ms is the wait-for-send time and works as smq_send.

<br><br>
`void *vsmq_recv(vSMQ q, int *dsize, int timeout_ms)`

Receives a message from the queue.

* q is the vSMQ object (queue)
* dsize will have written the size of the data previously written from vsmq_send.
If dsize is NULL, no size data will be written, but an error will also not occur.
* timeout_ms is the time to wait to receive data and works like smq_recv.

Returns a pointer to the memory previously copied into the queue. NULL is returned
if nothing is available. This memory *must* be freed using `free()`.

Receives a message from the queue.
* vsmq is object created by vsmq_create()

<br><br>
`int vsmq_destroy(vSMQ)`

Same as smq_destroy

<br><br>
`int vsmq_get_count(vSMQ)`

Same as smq_get_count

<br><br>
`void vsmq_wipe(vSMQ`)

Same as smq_wipe
<br><br>

## Extended and/or Unsupported items

As this code is primarily a simple implementation of queues, there are a number
of items which are unsupported or don't exist. In some worlds, we may consider
these a "TODO," but there is no plan to implement these features in this
repo.

* All items in the queue are assumed to have the same priority and, as such, 
there is no prioritization to queue items. For example, there is no concept
of ``high'' or ``low'' priority items. Having this feature will also imped
the performance of the queue overall, which may be mitigated depending upon
how it is implemented. The simple implementation offers excellent overall
raw performance. Both writes and reads essentially operate at O(1).

* There is no functionality for merging queues, splitting queues, or copying
queues.

* Each queue must have a queue object assigned and must be tracked through
some other means. There is no concept of a ``list of queues'' and, as such,
queues cannot be named or numbered.