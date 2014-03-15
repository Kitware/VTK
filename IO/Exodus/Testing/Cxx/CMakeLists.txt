# Tests with data
# VS6 builds do not handle out-of-range double assignment to float
# properly. Do not run TestMultiBlockExodusWrite on VS6 builds.
#
if(NOT CMAKE_GENERATOR MATCHES "Visual Studio 6")
  set(extra_tests
    TestMultiBlockExodusWrite.cxx
    )
endif()

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestExodusAttributes.cxx,NO_VALID,NO_OUTPUT
  TestExodusSideSets.cxx,NO_VALID,NO_OUTPUT
  TestInSituExodus.cxx,NO_VALID
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY)
