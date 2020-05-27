## New reader for Exodus files

VTK now supports reading Exodus files using `vtkIossReader`. vtkIossReader uses
the [Sierra IO System (Ioss)](http://gsjaardema.github.io/seacas/).
vtkIossReader produces a `vtkPartitionedDataSetCollection` as the output data
type together with a `vtkDataAssembly` to represent the logical structure of the
blocks in the file. `vtkIossReader` is intended to eventually replace
`vtkExodusIIReader`.
