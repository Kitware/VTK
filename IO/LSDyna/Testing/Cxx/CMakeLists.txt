vtk_add_test_cxx(vtkIOLSDynaCxxTests tests
  TestLSDynaReaderIncompleteFamily.cxx,NO_DATA,NO_VALID
  )

if (VTK_USE_LARGE_DATA)
  vtk_add_test_cxx(vtkIOLSDynaCxxTests tests
    TestLSDynaReader.cxx
    TestLSDynaReaderDeflection.cxx
    #TestLSDynaReaderNoDefl.cxx
    TestLSDynaReaderSPH.cxx
    TestLSDynaReaderUserIds.cxx,NO_VALID
    )
endif ()

vtk_test_cxx_executable(vtkIOLSDynaCxxTests tests)
