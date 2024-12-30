## Fix wide lines with cell colors for Apple silicon

You can now render thick lines and map colors using cell scalars on Apple silicon. Previously, Apple silicon
chips would only render the first half of the line segments with cell scalar color and the rest of the lines
would not be drawn at all. VTK now handles this bug in upstream Apple driver by patching the shader code at
runtime. VTK applies the patch only for Apple OpenGL over Metal driver.

You can check for the presence of this bug in Apple drivers with the
`bool vtkOpenGLRenderWindow::IsPrimIDBugPresent()` method.
