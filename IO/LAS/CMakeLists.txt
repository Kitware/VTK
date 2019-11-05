vtk_module_find_package(PACKAGE libLAS)

set(classes
  vtkLASReader)

vtk_module_add_module(VTK::IOLAS
  CLASSES ${classes})

set(liblas_boost_dependencies)
if (libLAS_VERSION VERSION_LESS "1.8.2")
  vtk_module_find_package(PACKAGE Boost
    COMPONENTS program_options thread system iostreams filesystem
    FORWARD_VERSION_REQ EXACT
    VERSION_VAR         "@Boost_MAJOR_VERSION@.@Boost_MINOR_VERSION@.@Boost_SUBMINOR_VERSION@")
  list(APPEND liblas_boost_dependencies
    Boost::filesystem
    Boost::iostreams
    Boost::program_options
    Boost::system
    Boost::thread)
endif ()

vtk_module_link(VTK::IOLAS
  PRIVATE
    # XXX(liblas-1.8.2): Use these when the minimum version is 1.8.2+.
    # Currently, the imported target names vary based on the platform. In
    # addition, the include interface is not set up properly. See
    # https://github.com/libLAS/libLAS/pull/153.
    # las las_c
    ${libLAS_LIBRARIES}
    ${liblas_boost_dependencies})
vtk_module_include(VTK::IOLAS
  PRIVATE
    ${libLAS_INCLUDE_DIRS})
