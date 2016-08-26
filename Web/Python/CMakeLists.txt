# Make sure VTK_WRAP_PYTHON is ON otherwise vtk-web can not exist/work
if(NOT VTK_WRAP_PYTHON)
   message(FATAL_ERROR "Web group can NOT work if VTK_WRAP_PYTHON is not ON.")
endif()

# autobahn (a dependency) only supports python 2.7 and 3.3+.  Make this a configure error.
if (PYTHON_MAJOR_VERSION EQUAL 3)
  if (PYTHON_MINOR_VERSION LESS 3)
    message(FATAL_ERROR "Web group cannot work with this Python version, please update to 2.7 or 3.3+")
  endif()
else()
  if (PYTHON_MINOR_VERSION LESS 7)
    message(FATAL_ERROR "Web group cannot work with this Python version, please update to 2.7 or 3.3+")
  endif()
endif()

include(vtkModuleMacrosPython)
# The RELATIVE make vtk_module_python_package() preserve the vtk/module package
# structure when building/installing. Otherwise, we'd just get the "web"
# package.
vtk_module_python_package(${vtk-module} "vtk/web"
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}")
