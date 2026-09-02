## Cube axes draws its 3D text labels again

`vtkCubeAxesActor` again draws axis labels and titles when `UseTextActor3D` is enabled. The labels
disappeared for the camera positions where the actor renders an extra gridlines-only axis, because
`HasTranslucentPolygonalGeometry()` queried the first axis of each direction instead of the axes
selected for rendering. When that first axis was the gridlines-only one, the actor reported no
translucent geometry, the renderer skipped its translucent pass, and the `vtkTextActor3D` based
labels — which only draw in that pass — were never rendered.
