vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE OpenTURNS
  VERSION 1.12)

# XXX(openturns-1.23): This is fixed upstream.
# Detect static builds of OT and warn that patches need applied to OT to truly
# work in this configuration.
foreach (openturns_library IN LISTS OPENTURNS_LIBRARY)
  if (NOT TARGET "${openturns_library}")
    continue ()
  endif ()

  get_property(openturns_type TARGET "${openturns_library}"
    PROPERTY TYPE)
  if (NOT openturns_type STREQUAL "STATIC_LIBRARY")
    continue ()
  endif ()

  get_property(openturns_compile_definitions TARGET "${openturns_library}"
    PROPERTY INTERFACE_COMPILE_DEFINITIONS)
  if (NOT "OT_STATIC" IN_LIST openturns_compile_definitions)
    if (NOT VTK_Filters_OpenTURNS_ONLY_BUILD_NEEDED)
      message(AUTHOR_WARNING
        "OpenTURNS does not have a complete usage requirements set up for "
        "static usage before its 1.23 release. Please backport the CMake "
        "changes from https://github.com/openturns/openturns/pull/2555 for a "
        "complete fix. If only a VTK build (and no subsequent
        `find_package(VTK)` calls), this message may be suppressed by setting "
        "`VTK_Filters_OpenTURNS_ONLY_BUILD_NEEDED=1`")
    endif ()

    set_property(TARGET "${openturns_library}" APPEND
      PROPERTY
        INTERFACE_COMPILE_DEFINITIONS "OT_STATIC")
  endif ()
endforeach ()

set(classes
  vtkOTDensityMap
  vtkOTFilter
  vtkOTKernelSmoothing
  vtkOTScatterPlotMatrix
  vtkOTUtilities)

vtk_object_factory_declare(
  BASE vtkScatterPlotMatrix
  OVERRIDE vtkOTScatterPlotMatrix)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKFILTERSOPENTURNS_EXPORT")

vtk_module_add_module(VTK::FiltersOpenTURNS
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})

vtk_module_link(VTK::FiltersOpenTURNS
  PRIVATE
    ${OPENTURNS_LIBRARY})
vtk_add_test_mangling(VTK::FiltersOpenTURNS)
