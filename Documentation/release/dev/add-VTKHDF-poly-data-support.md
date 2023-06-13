## VTKHDF: Support for `vtkPolyData`

The `VTKHDF` format now supports both static and transient `vtkPolyData` files.

Schematically, the extra `Steps` group that contains metadata dictating how to read the transient data looks like this:

![schema](poly_data_hdf_schema.png)

The design is heavily inspired from the `vtkUnstructuredGrid` format.
