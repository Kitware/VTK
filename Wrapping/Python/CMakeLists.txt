if (NOT DEFINED VTK_INSTALL_PYTHON_EXES)
  option(VTK_INSTALL_PYTHON_EXES "Install vtkpython and pvtkpython" ON)
  mark_as_advanced(VTK_INSTALL_PYTHON_EXES)
endif ()

add_executable(vtkpython
  vtkpython.rc
  vtkPythonAppInit.cxx)
target_link_libraries(vtkpython
  PRIVATE
    VTK::WrappingPythonCore
    VTK::PythonInterpreter
    VTK::Python
    VTK::vtkpythonmodules)
add_executable(VTK::vtkpython ALIAS vtkpython)
if (VTK_INSTALL_PYTHON_EXES)
  install(
    TARGETS     vtkpython
    EXPORT      VTK
    DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif ()

if (TARGET VTK::ParallelMPI)
  add_executable(pvtkpython
    vtkPythonAppInit.cxx)
  target_compile_definitions(pvtkpython
    PRIVATE
      VTK_COMPILED_USING_MPI)
  target_link_libraries(pvtkpython
    PRIVATE
      VTK::WrappingPythonCore
      VTK::PythonInterpreter
      VTK::ParallelMPI
      VTK::Python
      VTK::mpi
      VTK::vtkpythonmodules)
  add_executable(VTK::pvtkpython ALIAS pvtkpython)
  if (VTK_INSTALL_PYTHON_EXES)
    install(
      TARGETS     pvtkpython
      EXPORT      VTK
      DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endif ()
endif ()

if (FALSE)
# "Configure" files that need to be configured, including generation of the *.py
# files for each of the enabled VTK modules.
# We don't directly configure to `VTK_BUILD_PYTHON_MODULES_DIR` since of VS generator, the
# `VTK_BUILD_PYTHON_MODULES_DIR` depends on build configuration not known at configure time.
# So we configure into a temporary location and then copy those files over to
# `VTK_BUILD_PYTHON_MODULES_DIR` using a custom_command.

# Wrapping/Python/vtk/*.py
unset(VTK_PYTHON_IMPORT_ALL)
unset(configured_py_files)
foreach(module IN LISTS VTK_PYTHON_MODULES_AND_KITS)
  set(VTK_PYTHON_IMPORT_ALL
    "${VTK_PYTHON_IMPORT_ALL}from .${module} import *\n")
  configure_file(vtkmodules/module.py.in
    "${CMAKE_CURRENT_BINARY_DIR}/vtkmodules/${module}.py" @ONLY)
  list(APPEND configured_py_files "${CMAKE_CURRENT_BINARY_DIR}/vtkmodules/${module}.py")
endforeach()
configure_file(vtkmodules/all.py.in
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmodules/all.py" @ONLY)
list(APPEND configured_py_files "${CMAKE_CURRENT_BINARY_DIR}/vtkmodules/all.py")

# Kit module adapters
foreach(kit IN LISTS vtk_kits)
  set(_module_kit ${kit}${VTK_KIT_SUFFIX})
  foreach(dep IN LISTS _${kit}_modules)
    configure_file(vtkmodules/kit_module.py.in
      "${CMAKE_CURRENT_BINARY_DIR}/vtkmodules/${dep}.py" @ONLY)
    list(APPEND configured_py_files "${CMAKE_CURRENT_BINARY_DIR}/vtkmodules/${dep}.py")
  endforeach()
  unset(_module_kit)
endforeach()

# Now copy configured files to build-cfg specific dir.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/copy-complete"
  COMMAND ${CMAKE_COMMAND}
          -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/vtkmodules" "${VTK_BUILD_PYTHON_MODULES_DIR}/vtkmodules"
  COMMAND ${CMAKE_COMMAND}
          -E touch "${CMAKE_CURRENT_BINARY_DIR}/copy-complete"
  DEPENDS ${configured_py_files})


# Build all py files that form the `vtk` package.
vtk_python_package(vtkpython_pyc vtkmodules DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/copy-complete")
vtk_python_module(vtk_python_module vtk.py)

# Let's add a target that generates the `vtk` python package
# including all the platform dependent and independent modules.
set(module_Python_libs)
foreach(module IN LISTS VTK_PYTHON_MODULES_AND_KITS)
  list(APPEND module_Python_libs ${module}Python)
endforeach()

add_custom_target(vtk_python_package)
add_dependencies(vtk_python_package vtkpython_pyc ${module_Python_libs})
unset(module_Python_libs)

if(TARGET vtkpython)
  add_dependencies(vtkpython vtk_python_package)
endif()
if(TARGET pvtkpython)
  add_dependencies(pvtkpython vtk_python_package)
endif()

# If no runtime is to be installed then do not install python modules.
if(NOT VTK_INSTALL_NO_RUNTIME)
  # Install the conveniently configured python interpretters
  if(NOT VTK_INSTALL_NO_PYTHON_EXES AND VTK_ENABLE_VTKPYTHON)
    # Install the vtkpython executable
    install(TARGETS vtkpython
      DESTINATION ${VTK_INSTALL_RUNTIME_DIR})

    if(PVTKPYTHON_EXECUTABLE)
      # Install the mpi enabled vtkpython executable
      install(TARGETS pvtkpython
        DESTINATION ${VTK_INSTALL_RUNTIME_DIR})
    endif()
  endif()
endif()
endif ()
