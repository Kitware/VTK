## Enable rendering of larger polygonal composite datasets

`vtkCompositePolyDataMapper` now considers limitations in buffer sizes in the underlying graphics library when batching blocks together for rendering. It checks the maximum number of triangles that can be rendered by a single `vtkPolyDataMapper` and restricts batches to be smaller than that limit. This enables rendering of larger polygonal composite datasets.
