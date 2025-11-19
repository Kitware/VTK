# Fix macos crash when timers restart after interactor destruction

When the vtkCocoaRenderWindowInteractor destructed, it was not
stopping its timers, which could cause a crash if the event loop
was later restarted by a new interactor.  This has been fixed.
