# Coincident topology offsets scale with the capacity of the depth buffer

VTK now measures the `units` of coincident topology offsets in steps of the
depth buffer you render to, as `glPolygonOffset` does. VTK used to measure them
in steps of a 16-bit depth buffer. On a 24-bit or 32-bit float depth buffer,
that made the default line offset of -4 about 1000 steps, which let lines and
points show through thin walls in front of them, especially with a close near
clipping plane.

VTK now uses a default polygon offset factor of 1.0 instead of 0.0, which keeps
lines and points on slanted surfaces free of z-fighting. VTK moves each surface
fragment away from the camera by the change in its depth across one pixel, so
surfaces that face the camera barely move and slanted surfaces move more.

`vtkOpenGLPolyDataMapper`, `vtkGLSLModCoincidentTopology` and
`vtkWebGPUPolyDataMapper` all apply these offsets. If you set offsets yourself
with `vtkMapper::SetResolveCoincidentTopologyLineOffsetParameters` or a related
method, your `units` now shift lines and points much less than before. Increase
them if you relied on the previous amount.
