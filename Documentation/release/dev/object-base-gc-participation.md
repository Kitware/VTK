## `vtkObjectBase` and `vtkGarbageCollector` participation

Opting a class into the garbage collector now involves overriding the new
`UsesGarbageCollector()` method to return `true` instead of overriding
`Register()` and `UnRegister()` manually. Overriding these methods is still
supported, but will be removed in the future.
