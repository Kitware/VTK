vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE PDAL
  VERSION 2.0)

set(classes
  vtkPDALReader)

set(pdal_libraries)
if (PDAL_VERSION VERSION_LESS "2.6")
  set(pdal_libraries pdal_util)
  if (APPLE OR WIN32)
    list(APPEND pdal_libraries
      pdalcpp)
  else()
    list(APPEND pdal_libraries
      pdal_base)
  endif()
else ()
  list(APPEND pdal_libraries
    pdalcpp)
endif ()

vtk_module_add_module(VTK::IOPDAL
  CLASSES ${classes})
vtk_module_link(VTK::IOPDAL
  NO_KIT_EXPORT_IF_SHARED
  PRIVATE
    ${pdal_libraries})

# PDAL 2.0 didn't include interfaces to its exported targets.
# Resolved in https://github.com/PDAL/PDAL/issues/2262.
if (PDAL_VERSION VERSION_LESS "2.2")
  vtk_module_include(VTK::IOPDAL
    PRIVATE
      ${PDAL_INCLUDE_DIRS})
endif ()

vtk_add_test_mangling(VTK::IOPDAL)
