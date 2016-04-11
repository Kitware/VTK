## DIY2 is a block-parallel library

DIY2 is a block-parallel library for implementing scalable algorithms that can execute both
in-core and out-of-core. The same program can be executed with one or more threads per MPI
process, seamlessly combining distributed-memory message passing with shared-memory thread
parallelism.  The abstraction enabling these capabilities is block parallelism; blocks
and their message queues are mapped onto processing elements (MPI processes or threads) and are
migrated between memory and storage by the DIY2 runtime. Complex communication patterns,
including neighbor exchange, merge reduction, swap reduction, and all-to-all exchange, are
possible in- and out-of-core in DIY2.

## Licensing

DIY2 is released as open source software under a BSD style [license](./LICENSE.txt).

## Dependencies

DIY2 requires an MPI installation. We recommend [MPICH](http://www.mpich.org/).

## Download, build, install

- You can clone this repository, or

- You can download the [latest tarball](https://github.com/diatomic/diy2/archive/master.tar.gz).


DIY2 is a header-only library. It does not need to be built; you can simply
include it in your project. The examples can be built using `cmake` from the
top level directory.

## Documentation

[Doxygen pages](https://diatomic.github.io/diy2)

## Example

A simple DIY2 program, shown below, consists of the following components:

- `struct`s called blocks,
- a diy object called the `master`,
- a set of callback functions performed on each block by `master.foreach()`,
- optionally one or more message exchanges between the blocks by `master.exchange()`, and
- there may be other collectives and global reductions not shown below.

The callback functions (`enqueue_block()` and `average()` in the example below) are given the block
pointer and a communication proxy for the message exchange between blocks. It is usual for the
callback functions to enqueue or dequeue messages from the proxy, so that information can be
received and sent during rounds of message exchange.

```cpp
    // --- main program --- //

    struct Block { float local, average; };             // define your block structure

    Master master(world);                               // world = MPI_Comm
    ...                                                 // populate master with blocks
    master.foreach<Block>(&enqueue_local);              // call enqueue_local() for each block
    master.exchange();                                  // exchange enqueued data between blocks
    master.foreach<Block>(&average);                    // call average() for each block

    // --- callback functions --- //

    // enqueue block data prior to exchanging it
    void enqueue_local(Block* b,                        // one block
                       const Proxy& cp,                 // communication proxy
                                                        // i.e., the neighbor blocks with which
                                                        // this block communicates
                       void* aux)                       // user-defined additional arguments
    {
        for (size_t i = 0; i < cp.link()->size(); i++)  // for all neighbor blocks
            cp.enqueue(cp.link()->target(i), b->local); // enqueue the data to be sent
                                                        // to this neighbor block in the next
                                                        // exchange
    }

    // use the received data after exchanging it, in this case compute its average
    void average(Block* b,                              // one block
                 const Proxy& cp,                       // communication proxy
                                                        // i.e., the neighbor blocks with which
                                                        // this block communicates
                 void* aux)                             // user-defined additional arguments
    {
        float x, average = 0;
        for (size_t i = 0; i < cp.link()->size(); i++)  // for all neighbor blocks
        {
            cp.dequeue(cp.link()->target(i).gid, x);    // dequeue the data received from this
                                                        // neighbor block in the last exchange
            average += x;
        }
        b->average = average / cp.link()->size();
    }
```
