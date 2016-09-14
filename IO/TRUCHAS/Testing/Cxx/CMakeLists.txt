vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestTRUCHASReader.cxx,NO_VALID
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
    RENDERING_FACTORY
    )
