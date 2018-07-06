find_package(OpenTURNS 1.11 REQUIRED)
include(${OPENTURNS_USE_FILE})

set(Module_SRCS
    vtkOTDensityMap.cxx
    vtkOTFilter.cxx
    vtkOTKernelSmoothing.cxx
    vtkOTScatterPlotMatrix.cxx
    ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
    vtkOTUtilities.cxx)

include(vtkObjectFactory)
vtk_add_override(vtkScatterPlotMatrix vtkOTScatterPlotMatrix)
vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(vtkFiltersOpenTurns ${Module_SRCS})
target_include_directories(vtkFiltersOpenTurns PUBLIC ${OPENTURNS_INCLUDE_DIRS})
target_link_libraries(vtkFiltersOpenTurns LINK_PRIVATE ${OPENTURNS_LIBRARIES})
