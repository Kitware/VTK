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

vtk_add_test_cxx(vtkIOExodusCxxTests tests
  TestExodusAttributes.cxx,NO_VALID,NO_OUTPUT
  TestExodusIgnoreFileTime.cxx,NO_VALID,NO_OUTPUT
  TestExodusSideSets.cxx,NO_VALID,NO_OUTPUT
  ${extra_tests}
  )

list(APPEND tests
  TestExodusTetra15.cxx
  TestExodusWedge21.cxx
)

vtk_test_cxx_executable(vtkIOExodusCxxTests tests
  RENDERING_FACTORY)

ExternalData_add_test(VTKData
  NAME vtkIOExodusCxx-Tetra15
  COMMAND vtkIOExodusCxxTests TestExodusTetra15
    -D ${VTK_TEST_DATA_DIR}
    -V DATA{../Data/Baseline/TestExodusTetra15.png}
    -T "${VTK_TEST_OUTPUT_DIR}"
    DATA{../../../../Testing/Data/tetra15.g}
)

ExternalData_add_test(VTKData
  NAME vtkIOExodusCxx-Wedge21
  COMMAND vtkIOExodusCxxTests TestExodusWedge21
    -D ${VTK_TEST_DATA_DIR}
    -V DATA{../Data/Baseline/TestExodusWedge21.png}
    -T "${VTK_TEST_OUTPUT_DIR}"
    DATA{../../../../Testing/Data/wedge21.g}
)
