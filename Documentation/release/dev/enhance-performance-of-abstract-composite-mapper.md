## Enhance performance of vtkCompositePolyDataMapper

The `vtkCompositePolyDataMapper` in VTK::RenderingCore module was improved to efficiently
render large composite datasets. It performs as fast as `vtkCompositePolyDataMapper2`
in VTK::RenderingOpenGL2 module. You no longer need to use `vtkCompositePolyDataMapper2` for its
performance benefits.

This refactor significantly impacted few VTK modules:
- `vtkCompositePolyDataMapper` now has an API similar to `vtkCompositePolyDataMapper2`. You will most likely
 end up renaming all occurrences of `vtkCompositePolyDataMapper2` to `vtkCompositePolyDataMapper` in your C++/Python code.
- `vtkCompositeSurfaceLICMapper` derives `vtkCompositePolyDataMapper` instead of `vtkCompositePolyDataMapper2`.
- The OSPRay module uses `vtkCompositePolyDataMapper` instead of `vtkCompositePolyDataMapper2`.
- `vtkVtkJSSceneGraphSerializer` uses `vtkCompositePolyDataMapper` instead of `vtkCompositePolyDataMapper2`.

In order to minimize code duplication and maintenance burden, `vtkCompositePolyDataMapper2` and
`vtkvtkOSPRayCompositePolyDataMapper2Node` are slated for removal in a future release and no longer
used in VTK.
