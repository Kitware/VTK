# Configure files with settings for use by the build.
option(VTK_ENABLE_WRAPPING "Whether wrapping is available or not" ON)
mark_as_advanced(VTK_ENABLE_WRAPPING)

# Add the option for build the Python wrapping to VTK.
include(CMakeDependentOption)
cmake_dependent_option(VTK_WRAP_PYTHON "Should VTK Python wrapping be built?" OFF
  "VTK_ENABLE_WRAPPING" OFF)
if (DEFINED VTK_PYTHON_VERSION)
  if (NOT VTK_PYTHON_VERSION STREQUAL "3")
    message(FATAL_ERROR
      "Only Python3 is supported.")
  endif ()
  message(DEPRECATION
    "Manually specifying `VTK_PYTHON_VERSION` is deprecated as only Python3 "
    "is supported.")
  unset(VTK_PYTHON_VERSION)
  unset(VTK_PYTHON_VERSION CACHE)
endif ()

set(default_dll_paths)
if (NOT "$ENV{VTK_DLL_PATHS}" STREQUAL "")
  if (CMAKE_HOST_WIN32)
    file(TO_CMAKE_PATH "${vtk_dll_path}" default_dll_paths)
  else ()
    string(REPLACE ":" ";" default_dll_paths "$ENV{VTK_DLL_PATHS}")
  endif ()
endif ()
set(VTK_DLL_PATHS "${default_dll_paths}"
  CACHE STRING "DLL paths to use during Python module loading.")
mark_as_advanced(VTK_DLL_PATHS)

if (VTK_BUILD_TESTING OR VTK_WRAP_PYTHON)
  # VTK only supports a single Python version at a time, so make artifact
  # finding interactive.
  set(Python3_ARTIFACTS_INTERACTIVE ON)
  # Need PYTHON_EXECUTABLE for HeaderTesting or python wrapping
  find_package(Python3 QUIET COMPONENTS Interpreter)
endif()

if(VTK_WRAP_PYTHON)
  set(VTK_WRAP_PYTHON_EXE VTK::WrapPython)
  set(VTK_WRAP_PYTHON_INIT_EXE VTK::WrapPythonInit)
endif()

cmake_dependent_option(VTK_USE_TK "Build VTK with Tk support" OFF
  "VTK_WRAP_PYTHON" OFF)

cmake_dependent_option(VTK_WRAP_JAVA "Should VTK Java wrapping be built?" OFF
  "VTK_ENABLE_WRAPPING;NOT CMAKE_VERSION VERSION_LESS \"3.12\"" OFF)
if(VTK_WRAP_JAVA)
  set(VTK_WRAP_JAVA3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping/Java")
  # Wrapping executables.
  set(VTK_WRAP_JAVA_EXE  VTK::WrapJava)
  set(VTK_PARSE_JAVA_EXE VTK::ParseJava)

  # Java package location.
  set(VTK_JAVA_JAR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/vtk.jar")
  set(VTK_JAVA_HOME "${VTK_BINARY_DIR}/java/vtk")
  file(MAKE_DIRECTORY "${VTK_JAVA_HOME}")
endif()

cmake_dependent_option(VTK_WRAP_SERIALIZATION "Should VTK serailizer wrapping be built?" OFF "VTK_ENABLE_WRAPPING" OFF)
