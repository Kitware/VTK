include(vtkModuleMacrosPython)
# The RELATIVE make vtk_module_python_package() preserve the vtk/module package
# structure when building/installing. Otherwise, we'd just get the "web"
# package.
vtk_module_python_package(${vtk-module} "vtk/web"
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}")
