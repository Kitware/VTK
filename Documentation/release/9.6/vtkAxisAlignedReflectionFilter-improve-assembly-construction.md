## vtkAxisAlignedReflectionFilter: Improve Assembly Construction for PDC

When the input is a `vtkPartitionedDataSetCollection` which has a `vtkDataAssembly`, `vtkAxisAlignedReflectionFilter`
now constructs the output `vtkDataAssembly` by reflecting the input assembly. This enhancement ensures that the
hierarchical structure of the data is preserved in the output, making it easier to manage and visualize complex
datasets.
