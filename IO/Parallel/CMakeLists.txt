find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(Module_SRCS
  vtkEnSightWriter.cxx
  # vtkExodusIIWriter.cxx needs vtkMultiProcessController
  vtkPChacoReader.cxx
  vtkPCosmoReader.cxx
  vtkPDataSetReader.cxx
  vtkPDataSetWriter.cxx
  vtkPImageWriter.cxx
  vtkPNrrdReader.cxx
  vtkPOpenFOAMReader.cxx
  vtkPSLACReader.cxx
  vtkVPICReader.cxx
  vtkWindBladeReader.cxx
  vtkXMLPHierarchicalBoxDataWriter.cxx
  vtkXMLPMultiBlockDataWriter.cxx
  )

# Overrides for object factory.
set(parallel_overrides
  ImageWriter
  )

foreach(_override ${parallel_overrides})
  vtk_add_override(vtk${_override} vtkP${_override})
endforeach()

foreach(_class ${vtk_module_overrides})
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

# Create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
set_source_files_properties(
  ${vtk-module}ObjectFactory.cxx
  WRAP_EXCLUDE
  )

vtk_module_library(vtkIOParallel ${Module_SRCS})
