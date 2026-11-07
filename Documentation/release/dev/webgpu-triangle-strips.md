# WebGPU: triangle strips

The WebGPU backend now draws the triangle strips of a `vtkPolyData`. Until now it skipped
`vtkPolyData::GetStrips()` entirely, so the output of `vtkStripper`, or of any source that emits
strips, rendered with holes or not at all.

Strips go through the same compute pass that splits polygons into triangles, with both
`vtkPolyDataMapper` and `vtkCompositePolyDataMapper`. Each triangle keeps the cell id of its strip, counted after the vertices, lines and polygons as
everywhere else in VTK, so cell data colors the right strip and hardware selection reports the
right cell. Wireframe draws every edge of every triangle in a strip, as the OpenGL backend does.

## A single property texture

A texture set with `vtkProperty::SetTexture()` is now rendered when the actor has no texture of
its own and the property does not use PBR interpolation. OpenGL blends all of a property's
textures; WebGPU renders only one, and warns and ignores them when the property holds more than
one.
