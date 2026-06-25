# vtkConvertToMultiBlock from vtkPartitionedDataSetCollection: use assembly when possible

`vtkConvertToMultiBlock` now uses available meta-information from `vtkPartitionedDataSetCollection`'s `vtkDataAssembly` to build a multiblock that mirrors the assembly structure when possible.

This meta-information is added by assembly generation functions in `vtkDataAssemblyUtilities` when generating an assembly from a multiblock.

Assembly category name and values are now exposed in `vtkDataAssemblyUtilities.h`.
