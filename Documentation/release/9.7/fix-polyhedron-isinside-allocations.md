## vtkPolyhedron: remove per-call allocation from IsInside

`vtkPolyhedron::IsInside` triangulated faces with more than four vertices by
constructing a `vtkPolygon` and a `vtkIdList`, then repopulating that polygon's
`vtkPoints` and `vtkIdList` for every such face. `IsInside` is called once per
integration substep by the streamline and probe filters, so the allocation
churn landed directly in the hot path, and the cached `vtkPolyhedron` living in
`vtkGenericCell`'s cell store could strand those objects past leak checking.

Faces are now triangulated through `vtkPolygon::EarClipPolygon3D`, the shared
allocation-free ear clip already used by the rendering triangulators and the
polyhedron contour path, working on caller-owned scratch vectors. Results are
unchanged: the solid-angle sum is independent of how a planar face is
triangulated.

This compounds with the closest-point short circuit added earlier, which made
`EvaluatePosition` call `IsInside` on its fast path for every substep and so
raised how often the allocations were paid.
