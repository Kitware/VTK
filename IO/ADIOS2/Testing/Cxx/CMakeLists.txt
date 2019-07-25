find_package(ADIOS2)

vtk_add_test_cxx(vtkIOADIOS2VARCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  UnitTestIOADIOS2VAR.cxx
  TestIOADIOS2VAR_VTI3D.cxx
  TestIOADIOS2VAR_VTI3DRendering.cxx
  TestIOADIOS2VAR_VTU3D.cxx
  TestIOADIOS2VAR_VTU3DRendering.cxx
)

vtk_test_cxx_executable(vtkIOADIOS2VARCxxTests tests)
target_link_libraries(vtkIOADIOS2VARCxxTests
  PUBLIC
    adios2::adios2 )
