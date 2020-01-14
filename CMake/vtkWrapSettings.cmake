# Configure files with settings for use by the build.

# Add the option for build the Python wrapping to VTK.
option(VTK_WRAP_PYTHON "Should VTK Python wrapping be built?" OFF)
set(VTK_PYTHON_VERSION 2 CACHE STRING
  "Python version to use")
set_property(CACHE VTK_PYTHON_VERSION
  PROPERTY
    STRINGS "2;3")
include(CMakeDependentOption)
cmake_dependent_option(VTK_WHEEL_BUILD "Generate a build for a Python wheel" OFF
  "VTK_WRAP_PYTHON;VTK_PYTHON_VERSION EQUAL 3" OFF)

# Force reset of hints file location in cache if it was moved
if(VTK_WRAP_HINTS AND NOT EXISTS ${VTK_WRAP_HINTS})
  unset(VTK_WRAP_HINTS CACHE)
  unset(VTK_WRAP_HINTS)
endif()

if(BUILD_TESTING OR VTK_WRAP_PYTHON)
  # Need PYTHON_EXECUTABLE for HeaderTesting or python wrapping
  find_package("Python${VTK_PYTHON_VERSION}" QUIET COMPONENTS Interpreter)
endif()

if(VTK_WRAP_PYTHON)
  set(VTK_WRAP_PYTHON_EXE VTK::WrapPython)
  set(VTK_WRAP_PYTHON_INIT_EXE VTK::WrapPythonInit)
endif()

include(CMakeDependentOption)
cmake_dependent_option(VTK_USE_TK "Build VTK with Tk support" OFF
  "VTK_WRAP_PYTHON" OFF)

option(VTK_WRAP_JAVA "Should VTK Java wrapping be built?" OFF)
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
