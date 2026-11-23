# Conduit table protocol in vtkConduitSource

`vtkConduitSource` now reads the Conduit table blueprint. Turn on
`UseTableProtocol` and give it a node whose `values` children are columns
(data arrays or mcarrays, all of the same length) to get a `vtkTable`; an
empty node gives an empty `vtkTable`, so ranks without rows can pass one. A
node holding several named tables produces a `vtkPartitionedDataSetCollection`
with one table per partitioned dataset (a `vtkMultiBlockDataSet` with
`OutputMultiBlock`). Every rank must name the same tables, and an empty
child gives an empty table on a rank with no rows for it. Each table's
`state/fields` and `fields` become field data, and the assembly node applies
as for meshes.
`vtkConduitToDataObject::FillTable` exposes the same conversion directly.
