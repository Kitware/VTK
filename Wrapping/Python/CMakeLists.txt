if (NOT DEFINED VTK_INSTALL_PYTHON_EXES)
  option(VTK_INSTALL_PYTHON_EXES "Install vtkpython and pvtkpython" ON)
  mark_as_advanced(VTK_INSTALL_PYTHON_EXES)
endif ()

# Set up rpaths
set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
if (UNIX)
  file(RELATIVE_PATH vtkpython_relpath
    "/prefix/${CMAKE_INSTALL_BINDIR}"
    "/prefix/${CMAKE_INSTALL_LIBDIR}")
  if (APPLE)
    set(vtkpython_rpath_prefix
      "@executable_path")
  else ()
    set(vtkpython_rpath_prefix
      "$ORIGIN")
  endif ()

  list(APPEND CMAKE_INSTALL_RPATH
    "${vtkpython_rpath_prefix}/${vtkpython_relpath}")
endif ()

add_executable(vtkpython
  vtkpython.rc
  vtkPythonAppInit.cxx)
target_link_libraries(vtkpython
  PRIVATE
    VTK::WrappingPythonCore
    VTK::PythonInterpreter
    VTK::Python
    VTK::vtkpythonmodules
    VTK::vtksys)
add_executable(VTK::vtkpython ALIAS vtkpython)
if (VTK_INSTALL_PYTHON_EXES)
  install(
    TARGETS     vtkpython
    EXPORT      VTKPython
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
      EXPORT      VTKPython
      DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endif ()
endif ()

set(_vtk_python_imports)
foreach (_vtk_python_module IN LISTS vtk_python_wrapped_modules)
  get_property(_vtk_python_library_name
    TARGET    "${_vtk_python_module}"
    PROPERTY  "INTERFACE_vtk_module_library_name")
  string(APPEND _vtk_python_imports
    "from .${_vtk_python_library_name} import *\n")
endforeach ()

set(_vtk_python_files)
foreach (_vtk_module IN LISTS vtk_modules)
  _vtk_module_get_module_property("${_vtk_module}"
    PROPERTY "python_modules"
    VARIABLE _vtk_module_python_files)
  list(APPEND _vtk_python_files
    ${_vtk_module_python_files})
endforeach()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkmodules/all.py.in"
  "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules/all.py"
  @ONLY)
list(APPEND _vtk_python_files
  "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules/all.py")

if (BUILD_SHARED_LIBS)
  install(
    FILES       "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules/all.py"
    DESTINATION "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules")
endif ()

set(python_files
  vtk.py
  vtkmodules/__init__.py
  vtkmodules/gtk/GtkGLExtVTKRenderWindow.py
  vtkmodules/gtk/GtkGLExtVTKRenderWindowInteractor.py
  vtkmodules/gtk/GtkVTKRenderWindow.py
  vtkmodules/gtk/GtkVTKRenderWindowInteractor.py
  vtkmodules/gtk/__init__.py
  vtkmodules/numpy_interface/__init__.py
  vtkmodules/numpy_interface/algorithms.py
  vtkmodules/numpy_interface/dataset_adapter.py
  vtkmodules/numpy_interface/internal_algorithms.py
  vtkmodules/qt/QVTKRenderWindowInteractor.py
  vtkmodules/qt/__init__.py
  vtkmodules/test/BlackBox.py
  vtkmodules/test/Testing.py
  vtkmodules/test/__init__.py
  vtkmodules/tk/__init__.py
  vtkmodules/tk/vtkLoadPythonTkWidgets.py
  vtkmodules/tk/vtkTkImageViewerWidget.py
  vtkmodules/tk/vtkTkPhotoImage.py
  vtkmodules/tk/vtkTkRenderWidget.py
  vtkmodules/tk/vtkTkRenderWindowInteractor.py
  vtkmodules/util/__init__.py
  vtkmodules/util/colors.py
  vtkmodules/util/keys.py
  vtkmodules/util/misc.py
  vtkmodules/util/numpy_support.py
  vtkmodules/util/vtkAlgorithm.py
  vtkmodules/util/vtkConstants.py
  vtkmodules/util/vtkImageExportToArray.py
  vtkmodules/util/vtkImageImportFromArray.py
  vtkmodules/util/vtkMethodParser.py
  vtkmodules/util/vtkVariant.py
  vtkmodules/wx/__init__.py
  vtkmodules/wx/wxVTKRenderWindow.py
  vtkmodules/wx/wxVTKRenderWindowInteractor.py)

set(python_copied_modules)
foreach (python_file IN LISTS python_files)
  set(output_python_file
    "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file}")
  add_custom_command(
    OUTPUT  "${output_python_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
            "${output_python_file}"
    COMMENT "Copying ${python_file} to the binary directory")
  if (BUILD_SHARED_LIBS)
    get_filename_component(python_file_directory "${python_file}" DIRECTORY)
    install(
      FILES       "${python_file}"
      DESTINATION "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file_directory}"
      COMPONENT   "python")
  endif ()
  list(APPEND python_copied_modules
    "${output_python_file}")
endforeach ()

list(APPEND _vtk_python_files
  ${python_copied_modules})

set(_vtk_python_zip)
if (NOT BUILD_SHARED_LIBS)
  set(_vtk_python_zip "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/_vtk.zip")
  add_custom_command(
    OUTPUT            "${_vtk_python_zip}"
    COMMAND           ${CMAKE_COMMAND} -E tar "cfv"
                      "${_vtk_python_zip}" --format=zip ${_vtk_python_files}
    DEPENDS           ${_vtk_python_files}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}"
    COMMENT           "Creating _vtk.zip Python modules archive")
  install(
    FILES       "${_vtk_python_zip}"
    DESTINATION "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/"
    COMPONENT   "python")
endif ()

add_custom_target(vtk_python_copy ALL
  DEPENDS
    ${python_copied_modules}
    ${_vtk_python_zip})
