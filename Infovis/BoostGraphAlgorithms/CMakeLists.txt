set(classes
  vtkBoostBetweennessClustering
  vtkBoostBiconnectedComponents
  vtkBoostBrandesCentrality
  vtkBoostBreadthFirstSearch
  vtkBoostBreadthFirstSearchTree
  vtkBoostConnectedComponents
  vtkBoostDividedEdgeBundling
  vtkBoostExtractLargestComponent
  vtkBoostKruskalMinimumSpanningTree
  vtkBoostLogWeighting
  vtkBoostPrimMinimumSpanningTree
  vtkBoostRandomSparseArraySource
  vtkBoostSplitTableField)

set(headers
  vtkBoostGraphAdapter.h)

vtk_module_add_module(VTK::InfovisBoostGraphAlgorithms
  CLASSES ${classes}
  HEADERS ${headers})

vtk_module_find_package(
  PACKAGE Boost
  FORWARD_VERSION_REQ EXACT
  VERSION_VAR         "@Boost_MAJOR_VERSION@.@Boost_MINOR_VERSION@.@Boost_SUBMINOR_VERSION@")
vtk_module_link(VTK::InfovisBoostGraphAlgorithms
  PUBLIC
    Boost::boost)
