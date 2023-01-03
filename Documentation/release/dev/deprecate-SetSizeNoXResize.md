## Deprecation of vtkXOpenGLRenderWindow::SetSizeNoXResize()

The SetSizeNoXResize() method was added to vtkXOpenGLRenderWindow just prior
to VTK 9.0 for use by vtkXRenderWindowInteractor, but when the interactor
implementations were split out into the RenderingUI module for VTK 9, the
interactor was no longer able to call the method.
