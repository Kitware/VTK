## Add vtkThreadedCallbackQueue

`vtkThreadedCallbackQueue` is a new utility in VTK that allows to run functions in the background on
different threads. They are popped from the queue and invoked when a thread is available.
You can add any kind of functions with parameters to the queue using the `Push` method.
Controls are provided to change the number of threads, start and stop the queue. Note that all
controls run asynchronously. For example, the `Stop` method will likely end before the queue
is actually
stopped. This allows to change this queue's state without blocking the main pipeline. If it were not
the case, a heavy job currently running could make calling `Stop` potentially very long to execute.
Controls are serially run in the background by a dedicated thread.
