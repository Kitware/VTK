## Add EGL Mesa software rendering capabilities

VTK now provides software offscreen rendering without relying on OSMesa through the vtkEGLRenderWindow.

It would requires to build VTK with these options to ON:
- VTK_OPENGL_HAS_EGL
- VTK_USE_MESA_SOFTWARE_RENDERING

This functionality requires that mesa driver used support EGL_MESA_device_software extension.

Using VTK_USE_MESA_SOFTWARE_RENDERING forces MESA software rendering, preventing GPU usage for rendering.
