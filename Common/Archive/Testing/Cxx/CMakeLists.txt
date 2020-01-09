vtk_add_test_cxx(vtkCommonArchiveCxxTests tests
  TestBufferedRenderWindowExporter.cxx,NO_DATA,NO_VALID
  TestPartitionedRenderWindowExporter.cxx,NO_DATA,NO_VALID
  )
vtk_test_cxx_executable(vtkCommonArchiveCxxTests tests
  RENDERING_FACTORY
  )

find_package(LibArchive REQUIRED)

target_link_libraries(vtkCommonArchiveCxxTests
  PRIVATE
  LibArchive::LibArchive)
