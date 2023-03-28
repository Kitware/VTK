vtk_module_find_package(PACKAGE PDAL)

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
# XXX(pdal): PDAL should add include interfaces to its exported targets. See
# https://github.com/PDAL/PDAL/issues/2262.
vtk_module_include(VTK::IOPDAL
  PRIVATE
    ${PDAL_INCLUDE_DIRS})
