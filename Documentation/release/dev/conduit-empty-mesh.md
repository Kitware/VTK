# vtkConduitSource accepts an empty mesh node

`vtkConduitSource` gives a `vtkPartitionedDataSet` with no partitions for an
empty mesh node, also as a mesh of a multimesh, so a rank that holds no part of
a mesh can pass one. A mesh node without `topologies` or `coordsets` now fails
with an error instead of throwing.
