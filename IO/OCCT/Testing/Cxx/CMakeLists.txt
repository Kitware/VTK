vtk_add_test_cxx(vtkIOOCCTCxxTests tests
    TestOCCTReader.cxx,NO_OUTPUT
)

vtk_test_cxx_executable(vtkIOOCCTCxxTests tests)
