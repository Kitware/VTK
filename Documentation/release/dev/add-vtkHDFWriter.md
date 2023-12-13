## Support writing VTKHDF files

Previously, only reading VTKHDF files from VTK was supported. We introduce `vtkHDFWriter`,
capable of writing sequentially VTK data structures to disk in the VTKHDF format.
So far, the writer is capable of writing PolyData and Unstructured Grids, for both static and time-dependent data.
For now, composite datasets, multi-part data and distributed writing are not supported.
