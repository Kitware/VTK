# Make sure VTK_WRAP_PYTHON is ON otherwise vtk-web can not exist/work
if(NOT VTK_WRAP_PYTHON)
   message(FATAL_ERROR "Web group can NOT work if VTK_WRAP_PYTHON is not ON.")
endif()

include(vtkModuleMacrosPython)
# The RELATIVE make vtk_module_python_package() preserve the vtk/module package
# structure when building/installing. Otherwise, we'd just get the "web"
# package.
vtk_module_python_package(${vtk-module} "vtk/web"
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}")
