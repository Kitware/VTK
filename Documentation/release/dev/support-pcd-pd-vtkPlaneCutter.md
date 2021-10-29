## vtkPlaneCutter now supports vtkPartitionedDataSetCollection & vtkPartitionedDataSet

`vtkPlaneCutter` used to always generate a vtkMultiBlockDataSet regardless of input type.

Now `vtkPlaneCutter` decides what the output type will be based on the input type.

If input type is `vtkUniformGridAMR` or `vtkMultiBlockDataSet`, the output type will be `vtkMultiBlockDataSet`.
If input type is `vtkPartitionedDataSetCollection`, the output type will be `vtkPartitionedDataSetCollection`.
If input type is `vtkDataSet` or `vtkPartitionedDataSet`, the output type will be `vtkPartitionedDataSet`.

Additionally, two bugs related to CopyAllocate and CopyData of CellData were fixed, and a test for composite datasets
was added, which resolved the issue https://gitlab.kitware.com/paraview/paraview/-/issues/20795.

Finally, the RemapPointIds functor of `vtkRemoveUnusedPoints` has now been multithreaded properly, which resolved the
majority of the `vtkIOSSReader`-related tasks of the issue https://gitlab.kitware.com/vtk/vtk/-/issues/18222.
