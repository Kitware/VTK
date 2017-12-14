# Configure files with settings for use by the build.

# Add the option for build the Python wrapping to VTK.
option(VTK_WRAP_PYTHON "Should VTK Python wrapping be built?" OFF)
set(VTK_PYTHON_VERSION 2 CACHE STRING
  "Python version to use")
set_property(CACHE VTK_PYTHON_VERSION
  PROPERTY
    STRINGS "2;2.7;3;3.3;3.4;3.5;3.6;3.7;")

# Force reset of hints file location in cache if it was moved
if(VTK_WRAP_HINTS AND NOT EXISTS ${VTK_WRAP_HINTS})
  unset(VTK_WRAP_HINTS CACHE)
  unset(VTK_WRAP_HINTS)
endif()

if(BUILD_TESTING OR VTK_WRAP_PYTHON)
  # Need PYTHON_EXECUTABLE for HeaderTesting or python wrapping
  find_package(PythonInterp ${VTK_PYTHON_VERSION} QUIET)
  mark_as_advanced(PYTHON_EXECUTABLE)
endif()

if(VTK_WRAP_PYTHON)
  set(VTK_WRAP_PYTHON_EXE vtkWrapPython)
  set(VTK_WRAP_PYTHON_INIT_EXE vtkWrapPythonInit)
endif()
# Force the WrappingPythonCore module to on if wrapping is on
set(Module_vtkWrappingPythonCore ${VTK_WRAP_PYTHON}
  CACHE BOOL "Core Python wrapping library" FORCE)

option(VTK_WRAP_JAVA "Should VTK Java wrapping be built?" OFF)
if(VTK_WRAP_JAVA)
  set(VTK_WRAP_JAVA3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping/Java")
  # Wrapping executables.
  set(VTK_WRAP_JAVA_EXE  vtkWrapJava)
  set(VTK_PARSE_JAVA_EXE vtkParseJava)

  # Java package location.
  set(VTK_JAVA_JAR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/vtk.jar)
  set(VTK_JAVA_HOME ${VTK_BINARY_DIR}/java/vtk)
  file(MAKE_DIRECTORY ${VTK_JAVA_HOME})
endif()
set(Module_vtkWrappingJava ${VTK_WRAP_JAVA}
  CACHE BOOL "Core Java wrapping library" FORCE)

if(VTK_WRAP_PYTHON OR VTK_WRAP_JAVA OR VTK_WRAP_HIERARCHY)
  set(VTK_WRAP_HIERARCHY_EXE vtkWrapHierarchy)
endif()
