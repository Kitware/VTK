## Fix vtkPlaneCutter's Memory Leak and CopyStructure issues

`vtkPlaneCutter` now frees the sphere-trees if the input changes, and it can handle `vtkUniformGridAMR`.

Additionally, `vtkPlaneCutter` used to produce a vtkMultiPieceDataSet for each input (sub-)dataset. For
complex `vtkMultiBlockDataSet` this led to CopyStructure issues. That's why now `vtkAppendPolyData` is utilized to
merge the threaded results. Due to this change, the output type also changes as follows:

* If input type is `vtkMultiBlockDataSet`, the output type will be `vtkMultiBlockDataSet`.
* If input type is `vtkUniformGridAMR`, the output type will be `vtkPartitionedDataSetCollection`.
* If input type is `vtkPartitionedDataSetCollection`, the output type will be `vtkPartitionedDataSetCollection`.
* If input type is `vtkPartitionedDataSet`, the output type will be `vtkPartitionedDataSet`.
* If input type is `vtkDataSet`, the output type will be `vtkPolyData`.

In the past, the output type was as follows:

* If input type is `vtkUniformGridAMR` or `vtkMultiBlockDataSet`, the output type will be `vtkMultiBlockDataSet`.
* If input type is `vtkPartitionedDataSetCollection`, the output type will be
  `vtkPartitionedDataSetCollection`.
* If input type is `vtkDataSet` or `vtkPartitionedDataSet`, the output type will be `vtkPartitionedDataSet`.

This MR resolves issue https://gitlab.kitware.com/paraview/paraview/-/issues/21250.
