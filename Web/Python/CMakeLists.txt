vtk_module_impl()
vtk_module_export("")

find_package(PythonInterp)

include(vtkPythonPackages)

# Extract path where python module should be copied
get_property(pymodule_build_dir GLOBAL PROPERTY VTK_PYMODULE_BUILD_DIR)

set(vtkWebPython_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(vtkWebPython_BINARY_DIR "${pymodule_build_dir}/vtk/web")

build_python_package("vtkWebPython" ${vtkWebPython_SOURCE_DIR}
  ${vtkWebPython_BINARY_DIR})

# Make sure VTK_WRAP_PYTHON is ON otherwise vtk-web can not exist/work
if(NOT VTK_WRAP_PYTHON)
   message(FATAL_ERROR "Web group can NOT work if VTK_WRAP_PYTHON is not ON.")
endif()

install(DIRECTORY ${vtkWebPython_BINARY_DIR}
  DESTINATION "${VTK_INSTALL_PYTHON_MODULE_DIR}/vtk"
  COMPONENT Runtime)
