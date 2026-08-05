## Introduce `vtkAnariRenderWindow` offscreen

VTK can now use new `vtkAnariRenderWindow` in offscreen mode. This contribution is the first step to remove the OpenGL dependency in the ANARI module. As `vtkAnariPass` is the class that uses most of the OpenGL API, this allows to bypass this code path.
