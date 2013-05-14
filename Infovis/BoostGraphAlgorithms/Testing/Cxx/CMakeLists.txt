find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

vtk_add_test_cxx(
  # BoostArrayLogWeighting.cxx    # todo (unsatisfied deps)
  BoostArrayRandomSparseArraySource.cxx,NO_VALID
  TestBoostAdapter.cxx,NO_VALID
  TestBoostAlgorithms.cxx
  TestBoostBetweennessClustering.cxx,NO_VALID
  # TestBoostBrandesCentrality.cxx # todo (unsatisfied deps)
  TestBoostDividedEdgeBundling.cxx
  TestBoostExtractLargestComponent.cxx,NO_VALID
  TestBoostSplitTableField.cxx,NO_VALID
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
