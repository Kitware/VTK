# Fix point data of uniform surface/volume sampling in vtkMaskPoints

`vtkMaskPoints` now copies the point data of the point it actually sampled when
`RandomModeType` is `UNIFORM_SPATIAL_SURFACE` or `UNIFORM_SPATIAL_VOLUME`. In these
modes the output points carried the point data of the input point whose id happened to
match the index of the current sample. The point positions were correct, so the error
was only visible when the output was colored or otherwise processed by its point data.

The warning that these modes emit when the input contains no cell of the dimension they
sample now names that dimension, so it distinguishes surface sampling (2D cells) from
volume sampling (3D cells).
