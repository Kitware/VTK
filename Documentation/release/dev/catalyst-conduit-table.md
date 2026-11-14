# Conduit table protocol in vtkConduitSource

`vtkConduitSource` now reads the Conduit table blueprint. Turn on
`UseTableProtocol` and give it a node whose `values` children are columns
(data arrays or mcarrays, all of the same length) to get a `vtkTable`; an
empty node gives an empty `vtkTable`, so ranks without rows can pass one. A
node holding several named tables produces a `vtkPartitionedDataSetCollection`
with one table per partitioned dataset (a `vtkMultiBlockDataSet` with
`OutputMultiBlock`); every rank must then provide the same tables.
`vtkConduitToDataObject::FillTable` exposes the same conversion directly.
