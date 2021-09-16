## vtkVortexCore is now fully multithreaded

`vtkVortexCore` consists of 4 steps:
* `vtkGradientFilter`
* `MatrixVectorMultiply`
* `ComputeCriteria`
* `vtkParallelVectorsForVortexCore` which inherits `vtkParallelVectors`

`vtkGradientFilter` was already multithreaded, but `MatrixVectorMultiply`, `ComputeCriteria` and `vtkParallelVectors`
are now also multithreaded using `vtkSMPTools`.

Additionally, a bug related to the assignment of values of the additional criteria arrays was detected and resolved.
