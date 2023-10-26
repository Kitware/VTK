vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE PDAL
  VERSION 2.0)

set(classes
  vtkPDALReader)

set(pdal_libraries pdal_util)
if (APPLE OR WIN32)
  list(APPEND pdal_libraries
    pdalcpp)
else()
  list(APPEND pdal_libraries
    pdal_base)
endif()

vtk_module_add_module(VTK::IOPDAL
  CLASSES ${classes})
vtk_module_link(VTK::IOPDAL
  PRIVATE
    ${pdal_libraries})
vtk_add_test_mangling(VTK::IOPDAL)
