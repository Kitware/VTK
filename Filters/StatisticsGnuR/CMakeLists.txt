set(Module_SRCS
  vtkRRandomTableSource.cxx
  vtkRCalculatorFilter.cxx
  vtkRInterface.cxx
  vtkRAdapter.cxx
  vtkContingencyStatisticsGnuR.cxx
  vtkCorrelativeStatisticsGnuR.cxx
  vtkDescriptiveStatisticsGnuR.cxx
  vtkPCAStatisticsGnuR.cxx

  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
)

find_package(R)
include_directories(${R_INCLUDE_DIR})

# Configure the module specific settings into a module configured header.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkFiltersStatisticsGnuRConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkFiltersStatisticsGnuRConfigure.h)
set(${vtk-module}_HDRS
  ${CMAKE_CURRENT_BINARY_DIR}/vtkFiltersStatisticsGnuRConfigure.h
  )

# Set up some of the module specific preprocessor definitions.
set(${vtk-module}_EXPORT_CODE "#include \"vtkFiltersStatisticsGnuRConfigure.h\"")

set_source_files_properties(
  vtkRAdapter.cxx
  ${vtk-module}ObjectFactory
  WRAP_EXCLUDE
  )

# Now to generate our object factory.
set( vtk_module_overrides
  vtkContingencyStatistics
  vtkCorrelativeStatistics
  vtkDescriptiveStatistics
  vtkPCAStatistics
  )

# Now we iterate and create that class file...
foreach(_class ${vtk_module_overrides})
  set(vtk_module_${_class}_override ${_class}GnuR)
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"Override for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()

# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

vtk_module_library(vtkFiltersStatisticsGnuR ${Module_SRCS})

vtk_module_link_libraries(vtkFiltersStatisticsGnuR LINK_PRIVATE ${R_LIBRARIES})
