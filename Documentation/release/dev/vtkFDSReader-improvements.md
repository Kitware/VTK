## vtkFDSReader 3D boundary files (.bf)

The vtkFDSReader now supports 3D blockages and correctly applies patches on 3D boundaries.
NaN float values are applied where there is no data.

## vtkFDSReader performance improvements

The vtkFDSReader now reads boundary files faster, allowing to change
time step without having to read the whole boundary file again.
