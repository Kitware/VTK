## Add to the NetCDFCFReader the ability to use data from an XArray

An XArray can create a vtkNetCDFCFReader that uses its data, using
zero copy when possible to create a VTK dataset using ``` reader =
xarray_data.vtk.reader() ``` Any `vtkNetCDFCFReader` options can be
set (`FileName` is ignored), and the reader can be used as usual in a VTK
pipeline.
