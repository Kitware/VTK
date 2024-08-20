# Runtime settings

## OpenGL

On Linux and Windows, VTK will attempt to detect support for an OpenGL context backend at runtime
and create an appropriate subclass of `vtkOpenGLRenderWindow`. You can override this process by
specifying an environment variable `VTK_DEFAULT_OPENGL_WINDOW`. The possible values
are:

  1. `vtkXOpenGLRenderWindow` (Linux; applicable only when `VTK_USE_X` is `ON`, which is the default setting)
  2. `vtkWin32OpenGLRenderWindow` (Windows; applicable only when `VTK_USE_WIN32_OPENGL` is `ON`, which is the default setting)
  3. `vtkEGLRenderWindow` (applicable only when `VTK_OPENGL_HAS_EGL` is `ON`, which is the default setting)
  4. `vtkOSOpenGLRenderWindow` (OSMesa, requires that `osmesa.dll` or `libOSMesa.so` is installed)

Note: VTK does **not** support OSMesa on macOS, iOS, Android and WebAssembly platforms.
