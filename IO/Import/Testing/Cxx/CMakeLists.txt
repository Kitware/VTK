if(VTK_DATA_ROOT)
  vtk_add_test_cxx(NO_VALID
    TestVRMLNormals.cxx
    )
  vtk_test_cxx_executable(${vtk-module}CxxTests)
endif()
