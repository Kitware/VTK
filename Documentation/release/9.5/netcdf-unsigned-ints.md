## Support unsigned integers in netCDF files

Previously, netCDF readers such as `vtkNetCDFCFReader` did not support
several valid netCDF data types. In particular, the unsigned integers were
not supported. The readers now check for these types and load these
variables into the correct VTK array type.
