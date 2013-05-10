if(VTK_LARGE_DATA_ROOT)
  vtk_add_test_cxx(IO LARGE_DATA
    SLACMultipleModes.cxx
    SLACParticleReader.cxx
    SLACReaderLinear.cxx
    SLACReaderQuadratic.cxx
    TestMPASReader.cxx
    TestNetCDFCAMReader.cxx
    TestNetCDFPOPReader.cxx
    )
  vtk_test_cxx_executable(${vtk-module}CxxTests)
endif()
