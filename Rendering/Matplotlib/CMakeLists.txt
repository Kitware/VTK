# Check that matplotlib is available:
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import matplotlib"
  RESULT_VARIABLE IMPORT_MATPLOTLIB_EXITCODE)
set(MATPLOTLIB_FOUND FALSE)
if(${IMPORT_MATPLOTLIB_EXITCODE} EQUAL 0)
  set(MATPLOTLIB_FOUND TRUE)
endif()

if(NOT MATPLOTLIB_FOUND)
  message(FATAL_ERROR "Matplotlib not found! ('${PYTHON_EXECUTABLE} -c \"import matplotlib\"' failed.")
endif()

set(Module_SRCS
  vtkMatplotlibMathTextUtilities.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkMatplotlibObjectFactory.cxx
  )

set_source_files_properties(
  vtkMatplotlibObjectFactory
  vtkMatplotlibMathTextUtilities
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
