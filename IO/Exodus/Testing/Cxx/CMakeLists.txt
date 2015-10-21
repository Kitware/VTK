# Tests with data
# VS6 builds do not handle out-of-range double assignment to float
# properly. Do not run TestMultiBlockExodusWrite on VS6 builds.
#
if(NOT CMAKE_GENERATOR MATCHES "Visual Studio 6")
  set(extra_tests
    TestMultiBlockExodusWrite.cxx
    )
endif()

# We need typed arrays to be supported by the dispatcher to run the insitu test:
if(VTK_DISPATCH_TYPED_ARRAYS)
  set(extra_tests ${extra_tests}
    TestInSituExodus.cxx,NO_VALID
  )
endif()

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestExodusAttributes.cxx,NO_VALID,NO_OUTPUT
  TestExodusSideSets.cxx,NO_VALID,NO_OUTPUT
  ${extra_tests}
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY)
