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

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkmodules/all.py.in"
  "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules/all.py"
  @ONLY)
install(
  FILES       "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules/all.py"
  DESTINATION "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules")

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
  get_filename_component(python_file_directory "${python_file}" DIRECTORY)
  install(
    FILES       "${python_file}"
    DESTINATION "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file_directory}"
    COMPONENT   "python")
  list(APPEND python_copied_modules
    "${output_python_file}")
endforeach ()
add_custom_target(vtk_python_copy ALL
  DEPENDS
    ${python_copied_modules})
