## Improved OpenGL Debug Logging

You can now see detailed debug messages from OpenGL when VTK is built with `VTK_REPORT_OPENGL_ERRORS=ON` and `VTK_REPORT_OPENGL_ERRORS_IN_RELEASE_BUILDS=ON` for release builds. The `vtkOpenGLRenderWindow` utilizes the `GL_ARB_debug_output` extension to provide
more detailed error messages, that explain which methods caused an invalid operation. These messages offer you more clarity than the cryptic error codes returned by `glGetError`.

The `QVTKRenderWindowAdapter` class now creates a debug OpenGL context to enable debug output when `VTK_REPORT_OPENGL_ERRORS` is turned on.
