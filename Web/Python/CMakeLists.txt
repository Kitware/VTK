# Make sure VTK_WRAP_PYTHON is ON otherwise vtk-web can not exist/work
if(NOT VTK_WRAP_PYTHON)
   message(FATAL_ERROR "Web group can NOT work if VTK_WRAP_PYTHON is not ON.")
endif()

include(vtkModuleMacrosPython)
# The RELATIVE make vtk_module_python_package() preserve the vtk/module package
# structure when building/installing. Otherwise, we'd just get the "web"
# package.
vtk_module_python_package(${vtk-module} "vtkmodules/web"
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}")

set_property(GLOBAL APPEND
  PROPERTY
    vtk_required_python_modules "wslink>=0.1.3")
set_property(GLOBAL APPEND
  PROPERTY
    vtk_required_python_modules "Twisted>=17.5.0")
set_property(GLOBAL APPEND
  PROPERTY
    vtk_required_python_modules "autobahn>=17.7.1")

if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
