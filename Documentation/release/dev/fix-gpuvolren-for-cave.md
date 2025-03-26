## Fix GPU Ray Cast Volume Rendering with ModelTransformMatrix

For CAVE environments, the `vtkCamera` class provides a `ModelTransformMatrix` as
a means of manipulating all the objects in a scene.  However, actually using this
matrix effectively changes the position of the camera (or eye), which is used by the
`vtkOpenGLGPUVolumeRayCastMapper`.  This issue prevented CAVE users from interacting
with volume data in the CAVE, as written up [here](https://gitlab.kitware.com/paraview/paraview/-/issues/21471).

This change fixes the gpu volume rendering mapper by removing from developers
the responsibility to pass the cameraPos uniform so that it matches the actual
transformations applied to each vertex in the vertex shader.  Instead, the
effective eye position in dataspace is computed automatically.  Furthermore,
this is done on the cpu, saving a matrix/matrix multiply and several matrix/vector
multiplies in the fragment shader.
