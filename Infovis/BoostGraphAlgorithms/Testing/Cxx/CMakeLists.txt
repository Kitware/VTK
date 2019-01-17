vtk_add_test_cxx(vtkInfovisBoostGraphAlgorithmsCxxTests tests
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
vtk_test_cxx_executable(vtkInfovisBoostGraphAlgorithmsCxxTests tests)

find_package(Boost REQUIRED)
target_link_libraries(vtkInfovisBoostGraphAlgorithmsCxxTests
  PRIVATE
    Boost::boost)
