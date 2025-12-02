vtk_add_test_cxx(vtkIOUSDCxxTests tests
  TestUSDExporter.cxx,NO_DATA,NO_VALID
  )
vtk_test_cxx_executable(vtkIOUSDCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
