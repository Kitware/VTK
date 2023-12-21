## Support writing VTKHDF files

Previously, only reading VTKHDF files from VTK was supported. We introduce `vtkHDFWriter`,
capable of writing sequentially VTK data structures to disk in the VTKHDF format.
So far, the writer is capable of writing static and time-dependent data for:
- PolyData
- UnstructuredGrid
- PartitionedDataSetCollection and MultiBlockDataSet

For now, distributed writing are not supported.
