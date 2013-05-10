find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

vtk_add_test_cxx(Infovis
  # BoostArrayLogWeighting.cxx    # todo (unsatisfied deps)
  BoostArrayRandomSparseArraySource.cxx
  TestBoostAdapter.cxx
  TestBoostAlgorithms.cxx
  TestBoostBetweennessClustering.cxx
  # TestBoostBrandesCentrality.cxx # todo (unsatisfied deps)
  TestBoostDividedEdgeBundling.cxx
  TestBoostExtractLargestComponent.cxx
  TestBoostSplitTableField.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
