vtk_add_test_cxx(vtkIOEngysCxxTests tests
        NO_VALID
        TestBTSReader.cxx
        )

vtk_test_cxx_executable(vtkIOEngysCxxTests tests)
