
# OpenSlideReader Tests

vtk_add_test_cxx(vtkDomainsMicroscopyCxxTests tests
  TestOpenSlideReader.cxx
  "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Microscopy/small2.ndpi}"
  )

vtk_add_test_cxx(vtkDomainsMicroscopyCxxTests  tests
    TestOpenSlideReaderPartialTIF,TestOpenSlideReaderPartial.cxx
  "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Microscopy/pathology.tif}"
  )

vtk_test_cxx_executable(vtkDomainsMicroscopyCxxTests tests RENDERING_FACTORY)
