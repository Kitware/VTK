## Garbage collection in threads other than the main thread

`vtkGarbageCollector` defers collection only in the main thread, the thread that loaded VTK. In other threads, `DeferredCollectionPush()`, `DeferredCollectionPop()` and `Collect()` now do nothing, and objects are collected immediately, as they already were. Previously, these asserted in debug builds. In release builds, they changed the deferral of the main thread, and `Collect()` never returned while the main thread had deferred collections.

This lets code that runs VTK in a worker thread, e.g. ParaView Catalyst in Catalyst's asynchronous mode, use code that defers collection.
