## Add Wayland support forthe vtkEGLRenderWindow

vtkEGLRenderWindow supports native Wayland as backend which will be able to perform onscreen and offscreen rendering.

To use it, you need to set the new option `VTK_USE_WAYLAND_OPENGL` to `ON` (default is `OFF`).
This Wayland backend requires the `wayland-protocols` package when building VTK.
