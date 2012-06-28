set(vtkMetaIO_INCLUDE_DIRS
  ${CMAKE_CURRENT_BINARY_DIR}/vtkmetaio # metaIOConfig.h
  )
set(vtkMetaIO_LIBRARIES vtkmetaio)
vtk_module_export_info()
set(METAIO_FOR_VTK 1)
set(VTK_ZLIB_LIBRARIES ${vtkzlib_LIBRARIES})
add_subdirectory(vtkmetaio)
vtk_target(vtkmetaio NO_INSTALL)
