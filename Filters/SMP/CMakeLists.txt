set(Module_SRCS
  vtkSMPContourGrid.cxx
  vtkSMPMergePoints.cxx
  vtkSMPMergePolyDataHelper.cxx
  )

set_source_files_properties(
  vtkSMPMergePolyDataHelper
  WRAP_EXCLUDE
  )

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND Module_SRCS
    vtkSMPContourGridManyPieces.cxx
    vtkThreadedSynchronizedTemplates3D.cxx
    vtkThreadedSynchronizedTemplatesCutter3D.cxx
    vtkSMPTransform.cxx
    vtkSMPWarpVector.cxx)
endif()

vtk_module_library(vtkFiltersSMP ${Module_SRCS})
