## Improve JOGL Integration (JogAmp v2.6.0)

Used JOGL/JogAmp version:
- [JogAmp v2.6.0](https://jogamp.org/deployment/v2.6.0)
- [Downloading and Installing](https://jogamp.org/wiki/index.php?title=Downloading_and_installing_JOGL)

### Fix vtkGenericOpenGLRenderWindow::OpenGLInit()

`vtkGenericOpenGLRenderWindow::OpenGLInit()` must call OpenGLInitContext() before usage via `state->Reset()`
to ensure all OpenGL function pointer (glad) are set.

GL state `Reset` uses OpenGL functionality and w/o having initialized the glad dispatch table
application gives SIGSEGV calling a nullptr function.

### Seamless OpenGL usage between JOGL and VTK

Ensures JOGL and VTK utilizes the same OpenGL function pointer lookup.
The latter is essential to ensure same set of OpenGL functions are being used between JOGL and VTK
and further allows JOGL loading a custom loaded OpenGL library.
