## Add methods to clean up observers in vtkSession

The vtkSession C API now provides two methods to remove observers from objects in a session:

* `vtkSessionRemoveAllObservers()` removes all observers from a specific object
* `vtkSessionRemoveAllObserversFromAllObjects()` removes all observers from all objects in the session

These methods help you manage observer cleanup.

These methods are also exposed on the WASM session interface, so you can use these methods:

- `vtkStandaloneSession::UnObserveAll(vtkObjectHandle object)` removes all observers from a specific object
- `vtkStandaloneSession::UnObserveAllObjects()` removes all observers from all objects in the session
- `vtkRemoteSession::UnObserveAll(vtkObjectHandle object)` removes all observers from a specific object
- `vtkRemoteSession::UnObserveAllObjects()` removes all observers from all objects in the session
