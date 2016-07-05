set(Module_SRCS
  vtkBooleanTexture.cxx
  vtkCheckerboardSplatter.cxx
  vtkFastSplatter.cxx
  vtkGaussianSplatter.cxx
  vtkImageCursor3D.cxx
  vtkImageRectilinearWipe.cxx
  vtkImageToPoints.cxx
  vtkPointLoad.cxx
  vtkSampleFunction.cxx
  vtkShepardMethod.cxx
  vtkSliceCubes.cxx
  vtkSurfaceReconstructionFilter.cxx
  vtkTriangularTexture.cxx
  vtkVoxelModeller.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
