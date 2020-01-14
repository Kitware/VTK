find_package(Python3 COMPONENTS Interpreter)

execute_process(
  COMMAND "${Python3_EXECUTABLE}"
          -c "import sysconfig; print(sysconfig.get_config_var('SOABI'))"
  OUTPUT_VARIABLE vtk_soabi
  ERROR_VARIABLE  err
  RESULT_VARIABLE res
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to determine SOABI for Python implementation: ${err}")
endif ()

execute_process(
  COMMAND "${Python3_EXECUTABLE}"
          -c "from distutils import util; print(util.get_platform())"
  OUTPUT_VARIABLE python_platform
  ERROR_VARIABLE  err
  RESULT_VARIABLE res
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to determine platform for Python implementation: ${err}")
endif ()

set(wheel_data_dir
  "vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}.data")

# Set up the install tree for a wheel.
set(CMAKE_INSTALL_INCLUDEDIR
  "${wheel_data_dir}/headers")
set(vtk_cmake_destination
  "${wheel_data_dir}/headers/cmake")
set(CMAKE_INSTALL_SHAREDIR
  "${wheel_data_dir}/headers/cmake")
set(CMAKE_INSTALL_DATAROOTDIR
  "${wheel_data_dir}/share")
set(CMAKE_INSTALL_DOCDIR
  "${wheel_data_dir}/share")
set(vtk_hierarchy_destination_args
  HIERARCHY_DESTINATION "${wheel_data_dir}/headers/hierarchy")
set(setup_py_build_dir
  "build/lib.${python_platform}-${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
# Required for Windows DLL placement.
set(CMAKE_INSTALL_BINDIR
  # Must correlate with `vtk_module_wrap_python(PYTHON_PACKAGE)` argument
  "${setup_py_build_dir}/vtkmodules")
set(CMAKE_INSTALL_LIBDIR
  # Must correlate with `vtk_module_wrap_python(PYTHON_PACKAGE)` argument
  "${setup_py_build_dir}/vtkmodules")
set(VTK_PYTHON_SITE_PACKAGES_SUFFIX ".")
set(VTK_CUSTOM_LIBRARY_SUFFIX "")
set(VTK_INSTALL_SDK OFF)
set(VTK_INSTALL_PYTHON_EXES OFF)
set(BUILD_SHARED_LIBS ON)

# macOS loader settings.
set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR ON)
set(CMAKE_INSTALL_NAME_DIR "@rpath")
if (APPLE)
  list(APPEND CMAKE_INSTALL_RPATH
    "@loader_path")
elseif (UNIX)
  list(APPEND CMAKE_INSTALL_RPATH
    "$ORIGIN")
endif ()
set(VTK_PYTHON_OPTIONAL_LINK ON)

# ELF loader settings.
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

set(license_file
  "${CMAKE_SOURCE_DIR}/Copyright.txt")
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/setup.py.in"
  "${CMAKE_BINARY_DIR}/setup.py"
  @ONLY)
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/MANIFEST.in.in"
  "${CMAKE_BINARY_DIR}/MANIFEST.in"
  @ONLY)
configure_file(
  "${CMAKE_SOURCE_DIR}/README.md"
  "${CMAKE_BINARY_DIR}/README.md"
  COPYONLY)

unset(license_file)
unset(wheel_data_dir)
