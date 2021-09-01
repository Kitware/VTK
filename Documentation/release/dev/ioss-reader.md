## New reader for Exodus files

VTK now supports reading Exodus files using `vtkIOSSReader`. vtkIOSSReader uses
the [Sierra IO System (IOSS)](http://gsjaardema.github.io/seacas/).
vtkIOSSReader produces a `vtkPartitionedDataSetCollection` as the output data
type together with a `vtkDataAssembly` to represent the logical structure of the
blocks in the file. `vtkIOSSReader` is intended to eventually replace
`vtkExodusIIReader`.
