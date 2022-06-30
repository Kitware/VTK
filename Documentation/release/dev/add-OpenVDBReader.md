# OpenVDB Reader

You can now use `vtkOpenVDBReader` in the `IOOpenVDB` module to read .vdb files.
It will create a `vtkPartitionedDataSetCollection`, with one block per selected grid.
Each block is either a `vtkImageData` or a `vtkPolyData` depending on the nature of the grid.
