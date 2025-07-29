# Fix wide line rendering in GLES default polydata mapper.

Earlier, the `vtkOpenGLLowMemoryPolyDataMapper` rendered thick lines incorrectly. It appeared as an extruded cross, see [vtk/vtk#19580](https://gitlab.kitware.com/vtk/vtk/-/issues/19580).
This issue is now fixed in the mapper.
