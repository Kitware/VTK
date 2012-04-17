set(Module_SRCS
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
  vtkHyperTreeGenerator.cxx
  )

vtk_module_library(vtkFiltersHyperTree ${Module_SRCS})
