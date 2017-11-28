set(Module_SRCS
  vtkSMPContourGrid.cxx
  vtkSMPMergePoints.cxx
  vtkSMPMergePolyDataHelper.cxx
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
