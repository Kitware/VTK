set(Module_SRCS
  vtkBooleanTexture.cxx
  vtkShepardMethod.cxx
  vtkVoxelModeller.cxx
  vtkSurfaceReconstructionFilter.cxx
  vtkFastSplatter.cxx
  vtkGaussianSplatter.cxx
  vtkSampleFunction.cxx
  vtkPointLoad.cxx
  vtkImageCursor3D.cxx
  vtkImageRectilinearWipe.cxx
  vtkTriangularTexture.cxx
  vtkSliceCubes.cxx

  )

vtk_module_library(${vtk-module} ${Module_SRCS})
