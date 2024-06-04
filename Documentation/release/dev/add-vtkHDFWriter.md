## Support writing VTKHDF files

Previously, only reading VTKHDF files from VTK was supported. We introduce `vtkHDFWriter`,
capable of writing sequentially VTK data structures to disk in the VTKHDF format.
So far, the writer is capable of writing static and time-dependent data for vtkPolyData, vtkUnstructuredGrid
and their vtkPartitionedDataset versions.
vtkPartitionedDataSetCollection and vtkMultiBlockDataSet are also supported, without temporal support.
These composite types have the option to be written either in a single standalone file,
or as a collection of files: one describing the composite assembly structure, and
every other containing the data relative to a non-composite leaf.
Both are considered equivalent by the reader.

For now, distributed writing and multi-partition datasets not supported.

User can compress chunked dataset thanks to the CompressionLevel option.
