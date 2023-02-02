## New OpenGL ES 3.0 compatible polydata mapper

The `vtkOpenGLES30PolyDataMapper` supports polydata and composite dataset
rendering with OpenGL ES 3.0. If VTK was configured with `VTK_OPENGL_USE_GLES=ON`,
this mapper is an override for `vtkPolyDataMapper`. The new mapper derives
most functionality from `vtkOpenGLPolyDataMapper`. It uses non-indexed draw
commands to workaround access to primitive ID in shader programs.

`vtkCompositePolyDataMapper2` derives from `vtkOpenGLES30PolyDataMapper`
instead of `vtkOpenGLPolyDataMapper` when VTK targets OpenGL ES.
