## Convert any dataset to vtkPartitionedDataSetCollection and vice versa

`vtkPartitionedDataSetCollection` together with `vtkDataAssembly` is the new way
to representation hierarchical datasets in VTK which is intended to replace
`vtkMultiBlockDataSet` and `vtkMultiPieceDataSet`.

`vtkDataObjectToPartitionedDataSetCollection` is a new filter that lets you convert any dataset
into a `vtkPartitionedDataSetCollection` with ease. If input is a vtkMultiBlockDataSet, the hierarchical
organization is represented in the `vtkDataAssembly` associated with the generated output.

`vtkPartitionedDataSetCollectionToMultiBlockDataSet` and its parallel
counterpart `vtkPPartitionedDataSetCollectionToMultiBlockDataSet` are inverse of
`vtkDataObjectToPartitionedDataSetCollection` i.e. they can convert a
vtkPartitionedDataSetCollection to a `vtkMultiBlockDataSet`.
