## Fix crash when vtkConvertToPointCloud has null points in input

Previously, when `vtkConvertToPointCloud` had an input `vtkPointSet`
with a null `vtkPoints`, it would crash with a seg fault. The filter now
first checks whether the points object exists before attempting to copy
it.

Although having a `vtkPointSet` with a null `vtkPoints` sounds like an
error condition, it can happen fairly easily in MPI parallelism. When
data are unevenly distributed, it can happen that some ranks will get no
data. In this case, a pipeline object may create a `vtkUnstructuredGrid`
or `vtkPolyData` but never modify it. In this case, the freshly created
data object will have an uninitialized `vtkPoints` object.

One may correctly argue that an empty `vtkPolyData` should initialize its
`vtkPoints` object rather than return a null pointer. However, some classes
already rely on this behavior, and switching is difficult.
