## WebAssembly remote sessions drop deserialization constructors unsuitable for the web

The `vtkRemoteSession` now removes the deserialization constructors of classes
that cannot work on the web platform.
This replaces the earlier handling that only dropped the
`vtkOpenGLPolyDataMapper` and `vtkOpenGLPolyDataMapper2D` constructors.

`vtkGenericRenderWindowInteractor` is one such class. It is compiled on every
platform, so a state naming it used to be constructed verbatim even though it
does not integrate with the browser event loop. Dropping its exact-match
constructor makes those states fall back to the constructor of the nearest
superclass, `vtkRenderWindowInteractor`, whose object factory instantiates
`vtkWebAssemblyRenderWindowInteractor`. This is the same path already taken by
states naming other platform-specific interactors, such as
`vtkCocoaRenderWindowInteractor` or `vtkWin32RenderWindowInteractor`, which are
absent from web builds.
