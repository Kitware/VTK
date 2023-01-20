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

# The interpreters are not supported in wheel builds, so skip them.
if (NOT VTK_WHEEL_BUILD)
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
endif ()

# goes in __init__.py for when vtkmodules is BUILD_STATIC
set(_vtkmodules_static_import "_load_vtkmodules_static()")
if (BUILD_SHARED_LIBS)
    string(PREPEND _vtkmodules_static_import "#")
endif ()

set(_vtkmodules_all)
set(_vtk_python_imports)
set(_vtk_python_external_imports)
foreach (_vtk_python_module IN LISTS vtk_python_wrapped_modules)
  get_property(_vtk_python_library_name
    TARGET    "${_vtk_python_module}"
    PROPERTY  "INTERFACE_vtk_module_library_name")
  string(APPEND _vtkmodules_all
    "    \'${_vtk_python_library_name}\',\n")
  string(APPEND _vtk_python_imports
    "from .${_vtk_python_library_name} import *\n")
  string(APPEND _vtk_python_external_imports
    "from vtkmodules.${_vtk_python_library_name} import *\n")
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
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkmodules/generate_pyi.py"
  "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules/generate_pyi.py"
  COPYONLY)
list(APPEND _vtk_python_files
  "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules/generate_pyi.py")

if (BUILD_SHARED_LIBS)
  install(
    FILES       "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules/generate_pyi.py"
    DESTINATION "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules"
    COMPONENT   "python")
endif ()

set(python_files
  vtk.py.in
  vtkmodules/__init__.py.in
  vtkmodules/all.py.in
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
  vtkmodules/test/ErrorObserver.py
  vtkmodules/test/Testing.py
  vtkmodules/test/rtImageTest.py
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
  vtkmodules/util/pickle_support.py
  vtkmodules/util/vtkAlgorithm.py
  vtkmodules/util/vtkConstants.py
  vtkmodules/util/vtkImageExportToArray.py
  vtkmodules/util/vtkImageImportFromArray.py
  vtkmodules/util/vtkMethodParser.py
  vtkmodules/util/vtkVariant.py
  vtkmodules/wx/__init__.py
  vtkmodules/wx/wxVTKRenderWindow.py
  vtkmodules/wx/wxVTKRenderWindowInteractor.py)

foreach (python_file IN LISTS python_files)
  # add submodules that don't begin with "_" to _vtkmodules_all
  if (python_file MATCHES "^vtkmodules\/([A-Za-z][A-Za-z0-9_]*)(\/__init__)?[.]py")
    string(APPEND _vtkmodules_all "    \'${CMAKE_MATCH_1}\',\n")
  endif ()
endforeach ()

set(python_copied_modules)
foreach (python_file IN LISTS python_files)
  set(output_python_file
    "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file}")
  if (python_file MATCHES "\\.in$")
    string(REPLACE ".in" "" output_python_file "${output_python_file}")
    configure_file(
      "${python_file}"
      "${output_python_file}"
      @ONLY)
  else ()
    add_custom_command(
      OUTPUT  "${output_python_file}"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
              "${output_python_file}"
      COMMENT "Copying ${python_file} to the binary directory")
  endif ()
  if (BUILD_SHARED_LIBS)
    get_filename_component(python_file_directory "${python_file}" DIRECTORY)
    install(
      FILES       "${output_python_file}"
      DESTINATION "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file_directory}"
      COMPONENT   "python")
  endif ()
  list(APPEND python_copied_modules
    "${output_python_file}")
endforeach ()

set(vtk_build_paths_module
  "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules/_build_paths.py")
if (WIN32 AND VTK_DLL_PATHS)
  set(vtk_dll_paths_python)
  foreach (vtk_dll_path IN LISTS VTK_DLL_PATHS)
    file(TO_NATIVE_PATH "${vtk_dll_path}" vtk_dll_path_native)
    string(APPEND vtk_dll_paths_python
      "    r'${vtk_dll_path_native}',\n")
  endforeach ()
  file(WRITE "${vtk_build_paths_module}"
    "paths = [\n${vtk_dll_paths_python}]\n")
  # Add the file to the zip only if required in the install tree.
  if (NOT VTK_RELOCATABLE_INSTALL AND NOT VTK_UNIFIED_INSTALL_TREE)
    list(APPEND _vtk_python_files
      "${vtk_build_paths_module}")
  endif ()
elseif (EXISTS "${vtk_build_paths_module}")
  file(REMOVE "${vtk_build_paths_module}")
endif ()

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

if (VTK_BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
