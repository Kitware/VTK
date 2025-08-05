## Deprecate vtkUniformGrid methods

vtkUniformGrid is now an empty shell and contain no members and only
deprecated methods.
  - vtkUniformGrid::Initialize has been moved to vtkAMRBox::InitializeGrid
  - vtkUniformGrid::NewImageDataCopy has been removed
  - vtkUniformGrid::ComputeScalarRange has been moved to vtkImageData
