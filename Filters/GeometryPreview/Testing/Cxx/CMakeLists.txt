vtk_add_test_cxx(vtkFiltersGeometryPreviewCxxTests tests
  TestOctreeImageToPointSetFilter.cxx
  TestPointSetToOctreeImageFilter.cxx
  TestPointSetStreamer.cxx
  )

vtk_test_cxx_executable(vtkFiltersGeometryPreviewCxxTests tests)
