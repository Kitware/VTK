## vtkFDSReader 3D boundary files (.bf)

The vtkFDSReader now supports 3D blockages and correctly applies patches on 3D boundaries.
Each blockage patch is now a separate PartitionedDataSet, so that patches can be enabled independently in the data assembly.

## vtkFDSReader performance improvements

The vtkFDSReader now reads boundary files faster, allowing to change
time step without having to read the whole boundary file again.
