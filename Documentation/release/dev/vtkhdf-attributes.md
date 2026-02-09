## VTKHDF: Add support for dataset attributes

VTKHDF now supports adding data array attributes, such as normals, scalars, global ids, etc.
using the HDF5 attribute "Attribute" on field array datasets.

Both `vtkHDFWriter` and `vtkHDFReader` support this new feature.
With this change, VTKHDF version is now 2.6.
