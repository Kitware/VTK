find_package(OpenTURNS REQUIRED)
include(${OPENTURNS_USE_FILE})
include_directories(${OPENTURNS_INCLUDE_DIRS})
add_definitions(-DVTK_USE_OPENTURNS)

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
