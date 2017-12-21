set(Module_SRCS
  vtkMomentsTensor.h
  vtkComputeMoments.cxx
  vtkMomentInvariants.cxx
  vtkMomentsHelper.cxx
  vtkSimilarityBalls.cxx
  vtkReconstructFromMoments.cxx
  )

vtk_module_library(vtkFiltersMomentInvariants ${Module_SRCS})
