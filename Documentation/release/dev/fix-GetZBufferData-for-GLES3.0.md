# Fix incorrect values from vtkOpenGLRenderWindow::GetZBufferData in OpenGL ES 3.0

`vtkOpenGLRenderWindow::GetZBufferData` now returns the correct depth values in OpenGL ES 3.0
where the depth component is typically only 24-bit wide.
