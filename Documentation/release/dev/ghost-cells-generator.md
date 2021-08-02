## vtkGhostCellsGenerator: universal ghost cells generator

`vtkGhostCellsGenerator` is a new filter generating ghost cells that will replace legacy ones.
It uses DIY for MPI communication and can handle any `vtkMultiBlockDataSet`,
`vtkPartitionedDataSet`, and `vtkPartitionedDataSetCollection` that is filled with supported input
data set types. Types supported by this filter are `vtkImageData`, `vtkRectilinearGrid`,
`vtkStructuredGrid`, `vtkUnstructuredGrid` and `vtkPolyData`.

Ghosts are exchanged between data set types of same type (a `vtkImageData` can only exchange ghost
with another `vtkImageData` for instance). Polymorphic types work, but any additive geometry
information in the sub-type will not be processed (for instance, `vtkUniformGrid` inherits from
`vtkImageData`, hence `vtkUniformGrid` is processed as a `vtkImageData` in the pipeline). Types are
exchanged inside a `vtkMultiBlockDataSet` and inside a `vtkPartitionedDataSet`, but are not
exchanged between 2 different `vtkPartitionedDataSet` inside a `vtkPartitionedDataSetCollection`.

Legacy ghost cells generator, namely `vtkUnstructuredGridGhostCellsGenerator`,
`vtkPUnstructuredGridGhostDataGenerator`, `vtkStructuredGridGhostDataGenerator`,
`vtkPStructuredGridGhostDataGenerator`, `vtkUniformGridGhostDataGenerator`, and
`vtkPUniformGridGhostDataGenerator` are all now deprecated, and `vtkGhostCellsGenerator` should be
used instead.

`vtkHyperTreeGrid` and `vtkExplicitStructuredGrid` are not supported by this new filter,
but a ghost cells generator exists for `vtkHyperTreeGrid`,
namely `vtkHyperTreeGridGhostCellsGenerator`.
