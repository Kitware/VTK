## Support writing VTKHDF files

Previously, only reading VTKHDF files from VTK was supported. We introduce `vtkHDFWriter`,
capable of writing sequentially VTK data structures to disk in the VTKHDF format.
So far, the writer is capable of writing static and time-dependent data for:
- PolyData
- UnstructuredGrid
PartitionedDataSetCollection and MultiBlockDataSet are also supported, without transient support.

For now, distributed writing and multi-partition datasets not supported.
For PDC, only the first partition of each PartitionedDataset is written.
