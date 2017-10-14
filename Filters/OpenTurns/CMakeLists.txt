find_package(OpenTURNS 1.8 REQUIRED)
include(${OPENTURNS_USE_FILE})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkOTConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkOTConfig.h)

set(Module_SRCS
    vtkOTDensityMap.cxx
    vtkOTFilter.cxx
    vtkOTKernelSmoothing.cxx
    vtkOTScatterPlotMatrix.cxx
    ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
    vtkOTUtilities.cxx)

set_source_files_properties(vtkOTUtilities WRAP_EXCLUDE)

include(vtkObjectFactory)
vtk_add_override(vtkScatterPlotMatrix vtkOTScatterPlotMatrix)
vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(vtkFiltersOpenTurns ${Module_SRCS})
target_link_libraries(vtkFiltersOpenTurns LINK_PRIVATE ${OPENTURNS_LIBRARIES})
