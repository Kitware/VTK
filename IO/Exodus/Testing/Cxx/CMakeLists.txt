# Tests with data
# VS6 builds do not handle out-of-range double assignment to float
# properly. Do not run TestMultiBlockExodusWrite on VS6 builds.
#
vtk_add_test_cxx(TestExodusAttributes.cxx NO_VALID NO_OUTPUT)
vtk_add_test_cxx(TestExodusSideSets.cxx NO_VALID NO_OUTPUT)
if(NOT CMAKE_GENERATOR MATCHES "Visual Studio 6")
  vtk_add_test_cxx(
    TestMultiBlockExodusWrite.cxx
    )
endif()

vtk_add_test_cxx(NO_VALID
  TestInSituExodus.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
