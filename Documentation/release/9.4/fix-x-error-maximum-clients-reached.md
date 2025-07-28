# Fix X error about maximum clients reached

Fixes a bug in `vtkXOpenGLRenderWindow` that prevented users from instatianting more than about 300
render windows on Linux with X11.
