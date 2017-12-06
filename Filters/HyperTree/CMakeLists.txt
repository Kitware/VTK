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

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND Module_SRCS
    vtkClipHyperOctree.cxx
    vtkHyperOctreeClipCutPointsGrabber.cxx
    vtkHyperOctreeContourFilter.cxx
    vtkHyperOctreeCutter.cxx
    vtkHyperOctreeDepth.cxx
    vtkHyperOctreeDualGridContourFilter.cxx
    vtkHyperOctreeLimiter.cxx
    vtkHyperOctreeSampleFunction.cxx
    vtkHyperOctreeSurfaceFilter.cxx
    vtkHyperOctreeToUniformGridFilter.cxx
  )
endif()

vtk_module_library(vtkFiltersHyperTree ${Module_SRCS})
