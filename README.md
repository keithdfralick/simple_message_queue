# Simple Message Queue (SMQ / vSMQ)

Simple Message Queue (SMQ / VSMQ) provides a simple set of routines 
(written in C) which provide a thread-safe mechanism for sending 
messages between threads. Only 6 public (non-static) functions are
exposed and required to send the messages. Locking is performed
by the codebase itself.

## SMQ vs vSMQ

SMQ is provides for a fixed size of data to be written as a message,
where-as vSMQ provides an optional extension, allowing memory 
pointers to be used and passed around more easily from the message 
queue. Essentially, SMQ is the base code and vSMQ is a wrapper
providing more flexible access. 

