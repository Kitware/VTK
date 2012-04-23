find_package(Boost 1.40.0 REQUIRED
  COMPONENTS serialization mpi filesystem system graph_parallel)
include_directories(${Boost_INCLUDE_DIRS})

set(Module_SRCS
  vtkPBGLBreadthFirstSearch.cxx
  vtkPBGLCollapseGraph.cxx
  vtkPBGLCollapseParallelEdges.cxx
  vtkPBGLCollectGraph.cxx
  vtkPBGLConnectedComponents.cxx
  vtkPBGLDistributedGraphHelper.cxx
  vtkPBGLGraphSQLReader.cxx
  vtkPBGLMinimumSpanningTree.cxx
  vtkPBGLRandomGraphSource.cxx
  vtkPBGLRMATGraphSource.cxx
  vtkPBGLShortestPaths.cxx
  vtkPBGLVertexColoring.cxx
  )

vtk_module_library(vtkInfovisParallel ${Module_SRCS})
target_link_libraries(vtkInfovisParallel ${Boost_LIBRARIES})
