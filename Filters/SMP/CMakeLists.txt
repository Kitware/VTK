set(Module_SRCS
  vtkSMPContourGrid.cxx
  vtkSMPContourGridManyPieces.cxx
  vtkSMPMergePoints.cxx
  vtkSMPMergePolyDataHelper.cxx
  vtkThreadedSynchronizedTemplates3D.cxx
  vtkSMPTransform.cxx
  vtkSMPWarpVector.cxx
  )

set_source_files_properties(
  vtkSMPMergePolyDataHelper
  WRAP_EXCLUDE
  )

vtk_module_library(vtkFiltersSMP ${Module_SRCS})
