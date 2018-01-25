set(Module_SRCS
  vtkHyperTreeGridAxisClip.cxx
  vtkHyperTreeGridAxisCut.cxx
  vtkHyperTreeGridAxisReflection.cxx
  vtkHyperTreeGridCellCenters.cxx
  vtkHyperTreeGridContour.cxx
  vtkHyperTreeGridDepthLimiter.cxx
  vtkHyperTreeGridGeometry.cxx
  vtkHyperTreeGridPlaneCutter.cxx
  vtkHyperTreeGridThreshold.cxx
  vtkHyperTreeGridToUnstructuredGrid.cxx
  )

vtk_module_library(vtkFiltersHyperTree ${Module_SRCS})
