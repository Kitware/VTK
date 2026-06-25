### Widget reference tracking

VTK now maintains references to widgets (subclasses of `vtkInteractorObserver`) in the render window interactor when `vtkRenderWindowInteractor::TrackInteractorObserverInstances` is enabled. This option is off by default.

**Benefits:**
- Prevents premature widget destruction in WebAssembly applications.
- Enables easier scene serialization by making widgets reachable from the interactor
- Allows traversal from the interactor to all widgets via `interactor->GetInteractorObservers()`

**Important considerations:**
- The interactor used to observe (and still does) `DeleteEvent` to call `SetInteractor(nullptr)`, which can cause re-entrancy issues if widgets call this in their destructors while the interactor is being destructed.
- All VTK widgets have been updated to handle instance tracking
- External widget code may break when tracking is enabled
- Please test your code with this option enabled if you are interested in your application running in a webassembly environment, or wish to use serialization capabilities of VTK.
