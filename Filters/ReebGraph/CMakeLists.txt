find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(Module_SRCS
  vtkReebGraphSimplificationFilter.cxx
  vtkReebGraphSurfaceSkeletonFilter.cxx
  vtkReebGraphToJoinSplitTreeFilter.cxx
  vtkReebGraphVolumeSkeletonFilter.cxx
  vtkUnstructuredGridToReebGraphFilter.cxx
  )

vtk_module_library(vtkFiltersReebGraph ${Module_SRCS})
