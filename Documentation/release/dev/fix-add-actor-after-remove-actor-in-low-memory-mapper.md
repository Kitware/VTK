## Fix low memory polydata mapper when an actor is added after another is removed from a renderer

Fixed a bug with the OpenGLES `vtkOpenGLLowMemoryPolyDataMapper` that appeared when actor is added,
removed, and then re-added, the actor was not become visible a second time.
