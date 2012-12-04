# Check that python is both found and version >= 2.6
if(NOT PYTHON_EXECUTABLE)
  message(FATAL_ERROR "Cannot build RenderingMatplotlib: PYTHON_EXECUTABLE not set.")
endif()

if(${PYTHON_VERSION_STRING} VERSION_LESS "2.6")
  message(FATAL_ERROR "Cannot build RenderingMatplotlib: Python is too old. "
    "Required: >= 2.6, Found: ${PYTHON_VERSION_STRING}")
endif()

set(Module_SRCS
  vtkMatplotlibMathTextUtilities.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )

set_source_files_properties(
  ${vtk-module}ObjectFactory
  WRAP_EXCLUDE
  )

# Override some rendering classes. Create object factory:
list(APPEND vtk_module_overrides "vtkMathTextUtilities")
set(vtk_module_vtkMathTextUtilities_override "vtkMatplotlibMathTextUtilities")

# Now we iterate and create that class file...
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

# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

vtk_module_library(${vtk-module} ${Module_SRCS})
