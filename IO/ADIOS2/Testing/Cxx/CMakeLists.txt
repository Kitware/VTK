find_package(ADIOS2)

vtk_add_test_cxx(vtkIOADIOS2ReaderCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  UnitTestIOADIOS2.cxx
  TestIOADIOS2VTI3D.cxx
  TestIOADIOS2VTI3DRendering.cxx
)

vtk_test_cxx_executable(vtkIOADIOS2ReaderCxxTests tests)
target_link_libraries(vtkIOADIOS2ReaderCxxTests
  PUBLIC
    adios2::adios2 )
