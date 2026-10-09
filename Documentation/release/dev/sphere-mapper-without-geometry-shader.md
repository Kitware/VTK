# vtkOpenGLSphereMapper no longer needs a geometry shader

`vtkOpenGLSphereMapper` now derives from `vtkOpenGLLowMemoryPolyDataMapper` instead of
`vtkOpenGLPolyDataMapper`. Each point is drawn as an instanced, camera-facing quad that the
vertex shader expands, so the mapper no longer needs a geometry shader and works with
OpenGL ES 3.0 and WebGL2.

A few behaviors changed:

* The actor's opacity is now applied once. Previously, translucent spheres were drawn with the
  square of the opacity, so an opacity of 0.5 looked like 0.25.
* Every point is drawn as a sphere even when the input has no cells. Previously, nothing was
  drawn for an input without cells.
* Only the first component of the scale array is used as the radius.

If you subclassed `vtkOpenGLSphereMapper`, note that the overrides of the
`vtkOpenGLPolyDataMapper` shader and buffer methods (`GetShaderTemplate`, `ReplaceShaderValues`,
`BuildBufferObjects`, `CreateVBO` and friends) are gone.

`vtkOpenGLLowMemoryPolyDataMapper` has a new protected `DrawPointsAsQuads` flag that draws each
vertex as an instanced 4-vertex triangle strip, and its shader replacement methods are now virtual.
