# Conduit table protocol in vtkConduitSource

`vtkConduitSource` now reads the Conduit table blueprint. Turn on
`UseTableProtocol` and give it a node whose `values` children are columns
(data arrays or mcarrays of the same length) to get a `vtkTable`; a node
holding several named tables produces a `vtkMultiBlockDataSet` with one
`vtkTable` per block. `vtkConduitToDataObject::FillTable` exposes the same
conversion directly.
