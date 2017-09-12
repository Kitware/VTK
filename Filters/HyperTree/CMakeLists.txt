set(Module_SRCS
  vtkClipHyperOctree.cxx
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

vtk_module_library(vtkFiltersHyperTree ${Module_SRCS})
