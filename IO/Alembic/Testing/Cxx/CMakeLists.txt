vtk_add_test_cxx(vtkIOAlembicCxxTests tests
  TestAlembicExporter.cxx,NO_DATA,NO_VALID
  )
vtk_test_cxx_executable(vtkIOAlembicCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
