if (VTK_DATA_ROOT)
  vtk_add_test_cxx(
    # TestImageReader2Factory.cxx   # fixme (deps not satisfied)
    TestNrrdReader.cxx
    )
  set(TestMetaIO_ARGS ${VTK_DATA_ROOT}/Data/HeadMRVolume.mhd)
  vtk_add_test_cxx(NO_DATA NO_VALID NO_OUTPUT
    TestMetaIO.cxx
    )
  vtk_test_cxx_executable(${vtk-module}CxxTests)
endif()
