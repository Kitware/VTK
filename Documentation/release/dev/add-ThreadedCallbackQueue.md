## Add vtkThreadedCallbackQueue

`vtkThreadedCallbackQueue` is a new utility in VTK that allows to run functions in the background on
different threads. They are popped from the queue and invoked when a thread is available.
You can add any kind of functions with parameters to the queue using the `Push` method.
Controls are provided to change the number of threads in an asynchronous way.

The `Push` method returns a `vtkSmartPointer<vtkThreadedCallbackQueue::vtkSharedFutureBase>`, which
lets the user synchronize tasks. The shared futures implement a function `Wait()`, which will
block the current thread until the task associated with the token has terminated. The shared
future, in its dynamic type, also provides access to the returned value from the task that
is associated with it.

Another method is provided to which you can pass a container of tokens: `PushDependent`. The task
pushed through this method will execute once all tokens' associated task has terminated.
