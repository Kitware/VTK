# Force some VTK options for wheels.
set(VTK_BUILD_TESTING OFF)
set(VTK_LEGACY_SILENT ON)
set(VTK_ENABLE_WRAPPING ON)
set(VTK_WRAP_PYTHON ON)
set(Python3_ARTIFACTS_INTERACTIVE ON)
# Disable the PythonInterpreter module; there's not much use for it within a
# Python wheel as the interpreter should be managed externally.
set(VTK_MODULE_ENABLE_VTK_PythonInterpreter NO)

if (UNIX AND NOT APPLE)
  # On Linux, prefer Legacy OpenGL library. We will revisit
  # this when all distributions will provides GLVND libraries by
  # default.
  if (NOT DEFINED OpenGL_GL_PREFERENCE OR OpenGL_GL_PREFERENCE STREQUAL "")
    set(OpenGL_GL_PREFERENCE "LEGACY")
  endif ()
endif ()

find_package(Python3 COMPONENTS Interpreter Development.Module)
set_property(GLOBAL PROPERTY _vtk_python_soabi "${Python3_SOABI}")

set(VTK_VERSION_SUFFIX "dev0"
  CACHE STRING "Suffix to use for the wheel version (e.g, 'dev0')")
mark_as_advanced(VTK_VERSION_SUFFIX)
set(VTK_DIST_NAME_SUFFIX ""
  CACHE STRING "Suffix to use for the wheel distribution name (e.g., 'osmesa')")
mark_as_advanced(VTK_DIST_NAME_SUFFIX)

execute_process(
  COMMAND "${Python3_EXECUTABLE}"
          "${CMAKE_CURRENT_LIST_DIR}/wheel_extract_platlib.py"
  OUTPUT_VARIABLE build_platlib
  ERROR_VARIABLE  err
  RESULT_VARIABLE res
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to determine the platform build directory: ${err}")
elseif (err)
  message(AUTHOR_WARNING
    "Platform build directory warning: ${err}")
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
  "${build_platlib}")
# Required for shared library placement.
if (WIN32)
  # Defaults are fine; handled by `delvewheel`.
elseif (APPLE)
  set(CMAKE_INSTALL_LIBDIR
    # Store libraries in a subdirectory here.
    "${setup_py_build_dir}/vtkmodules/.dylibs")
else ()
  set(CMAKE_INSTALL_LIBDIR
    # Linux bundles what libraries we have when they're put beside the modules.
    "${setup_py_build_dir}/vtkmodules")
endif ()
set(VTK_PYTHON_SITE_PACKAGES_SUFFIX ".")
if (WIN32)
  set(VTK_CUSTOM_LIBRARY_SUFFIX "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")
else()
  set(VTK_CUSTOM_LIBRARY_SUFFIX "")
endif()
if(NOT DEFINED VTK_INSTALL_SDK)
  set(VTK_INSTALL_SDK OFF)
endif()
set(VTK_INSTALL_PYTHON_EXES OFF)
set(BUILD_SHARED_LIBS ON)

if (APPLE)
  # macOS loader settings.
  set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR ON)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
  set(CMAKE_INSTALL_NAME_DIR "@rpath")
  list(APPEND CMAKE_INSTALL_RPATH
    "@loader_path")
elseif (UNIX)
  # ELF loader settings.
  set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
  list(APPEND CMAKE_INSTALL_RPATH
    "$ORIGIN")
endif ()
set(VTK_PYTHON_OPTIONAL_LINK ON)

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/setup.py.in"
  "${CMAKE_BINARY_DIR}/setup.py"
  @ONLY)
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/MANIFEST.in.in"
  "${CMAKE_BINARY_DIR}/MANIFEST.in"
  @ONLY)
configure_file(
  "${CMAKE_SOURCE_DIR}/Copyright.txt"
  "${CMAKE_BINARY_DIR}/LICENSE"
  COPYONLY)
configure_file(
  "${CMAKE_SOURCE_DIR}/README.md"
  "${CMAKE_BINARY_DIR}/README.md"
  COPYONLY)

unset(license_file)
unset(wheel_data_dir)
