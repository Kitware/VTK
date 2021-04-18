
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
  TestMultiBlockExodusWrite.cxx
  TestExodusTetra15.cxx
  TestExodusWedge18.cxx
  TestExodusWedge21.cxx
  ${extra_tests}
  )

list(APPEND tests
)

vtk_test_cxx_executable(vtkIOExodusCxxTests tests
  RENDERING_FACTORY)

ExternalData_add_test(VTKData
  NAME VTK::IOExodusCxx-Tetra15
  COMMAND vtkIOExodusCxxTests TestExodusTetra15
    -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -V DATA{../Data/Baseline/TestExodusTetra15.png}
    -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}"
    DATA{../../../../Testing/Data/tetra15.g}
)

ExternalData_add_test(VTKData
  NAME VTK::IOExodusCxx-Wedge18
  COMMAND vtkIOExodusCxxTests TestExodusWedge18
    -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -V DATA{../Data/Baseline/TestExodusWedge18.png}
    -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}"
    DATA{../../../../Testing/Data/wedge18.e}
)

ExternalData_add_test(VTKData
  NAME VTK::IOExodusCxx-Wedge21
  COMMAND vtkIOExodusCxxTests TestExodusWedge21
    -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -V DATA{../Data/Baseline/TestExodusWedge21.png}
    -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}"
    DATA{../../../../Testing/Data/wedge21.g}
)
