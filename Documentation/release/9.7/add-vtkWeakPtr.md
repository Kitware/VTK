## vtkWeakPtr

There is now a `vtkWeakPtr<T>`, a weak reference to `vtkObjectBase` instances,
which is thread-safe to management of the underlying object on other threads
(versus `vtkWeakPointer<T>` which is not thread-safe at all).

Note that this is a separate class because `vtkWeakPointer`'s API cannot be
supported in a thread-safe way. Instead of stuffing a thread-safe
implementation inside of it while also allowing the thread-unaware API
available, this new class is available to make sure that all uses are
thread-aware.
