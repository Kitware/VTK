set(extra_tests)
# We need soa arrays to be supported by the dispatcher to run the insitu test:
if(VTK_DISPATCH_SOA_ARRAYS)
  list(APPEND extra_tests
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

vtk_test_cxx_executable(vtkIOExodusCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
