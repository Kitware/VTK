# Dynamic X11 Function Loading in VTK

VTK now supports dynamic loading of X11 functions through the new `VTK::x11` module. You no longer need a hard X11 dependency for `vtkXRenderWindowInteractor` and `vtkXOpenGLRenderWindow`. The glad module has been refactored to use `vtkX11Functions`. When constructing `vtkXRenderWindowInteractor`, or `vtkXOpenGLRenderWindow`, symbols from `libX11` are loaded dynamically. This lets you use VTK with EGL/OSMesa on a system without `libX11` package.
