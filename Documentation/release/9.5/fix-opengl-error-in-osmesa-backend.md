# Fix OpenGL error in OSMesa backend

Fixed a GL_INVALID_ENUM error that occurred in `vtkOSOpenGLRenderWindow::ReportCapabilities`
because it used an unsupported method for querying `GL_EXTENSIONS` property.
