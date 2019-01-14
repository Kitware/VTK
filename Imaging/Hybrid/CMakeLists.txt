set(classes
  vtkBooleanTexture
  vtkCheckerboardSplatter
  vtkFastSplatter
  vtkGaussianSplatter
  vtkImageCursor3D
  vtkImageRectilinearWipe
  vtkImageToPoints
  vtkPointLoad
  vtkSampleFunction
  vtkShepardMethod
  vtkSliceCubes
  vtkSurfaceReconstructionFilter
  vtkTriangularTexture
  vtkVoxelModeller)

vtk_module_add_module(VTK::ImagingHybrid
  CLASSES ${classes})
