## Interpolate input dataset's field variables to vortex cores

`vtkVortexCore`'s output points now include the interpolated variables of the input points.
To accomplish that, `vtkParallelVectors`, which is called inside `vtkVortexCore`, calculates the
interpolation weights while visiting the triangles of each cell, and later on, uses the weights to
calculate the interpolation values for each variable.
