if(VTK_DATA_ROOT)
  vtk_add_test_cxx(TestLegacyCompositeDataReaderWriter.cxx Parallel)
  vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
endif()
