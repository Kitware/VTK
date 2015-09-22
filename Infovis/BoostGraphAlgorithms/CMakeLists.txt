find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(Module_SRCS
  vtkBoostBetweennessClustering.cxx
  vtkBoostBiconnectedComponents.cxx
  vtkBoostBrandesCentrality.cxx
  vtkBoostBreadthFirstSearch.cxx
  vtkBoostBreadthFirstSearchTree.cxx
  vtkBoostConnectedComponents.cxx
  vtkBoostDividedEdgeBundling.cxx
  vtkBoostExtractLargestComponent.cxx
  vtkBoostKruskalMinimumSpanningTree.cxx
  vtkBoostLogWeighting.cxx
  vtkBoostPrimMinimumSpanningTree.cxx
  vtkBoostRandomSparseArraySource.cxx
  vtkBoostSplitTableField.cxx

  vtkBoostGraphAdapter.h
  )

set_source_files_properties(
  vtkBoostGraphAdapter
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkInfovisBoostGraphAlgorithms ${Module_SRCS})
