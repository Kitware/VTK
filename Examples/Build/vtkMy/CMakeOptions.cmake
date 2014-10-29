#
# Configure output paths for libraries and executables.
#
set(LIBRARY_OUTPUT_PATH ${VTKMY_BINARY_DIR}/bin CACHE PATH
    "Single output directory for building all libraries.")
set(EXECUTABLE_OUTPUT_PATH ${VTKMY_BINARY_DIR}/bin CACHE PATH
    "Single output directory for building all executables.")
mark_as_advanced(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)

#
# Try to find VTK and include its settings (otherwise complain)
#
if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif()

#
# Build shared libs ?
#
# Defaults to the same VTK setting.
#

# Standard CMake option for building libraries shared or static by default.
option(BUILD_SHARED_LIBS
       "Build with shared libraries."
       ${VTK_BUILD_SHARED_LIBS})
# Copy the CMake option to a setting with VTKMY_ prefix for use in
# our project.  This name is used in vtkmyConfigure.h.in.
set(VTKMY_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

# If this is a build tree, provide an option for putting
# this project's executables and libraries in with VTK's.
if (EXISTS ${VTK_DIR}/bin)
  option(USE_VTK_OUTPUT_PATHS
         "Use VTK's output directory for this project's executables and libraries."
         OFF)
  MARK_AS_ADVANCED (USE_VTK_OUTPUT_PATHS)
  if (USE_VTK_OUTPUT_PATHS)
    set (LIBRARY_OUTPUT_PATH ${VTK_DIR}/bin)
    set (EXECUTABLE_OUTPUT_PATH ${VTK_DIR}/bin)
  endif ()
endif ()


#
# Wrap Tcl, Java, Python
#
# Rational: even if your VTK was wrapped, it does not mean that you want to
# wrap your own local classes.
# Default value is OFF as the VTK cache might have set them to ON but
# the wrappers might not be present (or yet not found).
#

#
# Tcl
#

if (VTK_WRAP_TCL)

  option(VTKMY_WRAP_TCL
         "Wrap classes into the TCL interpreted language."
         ON)

  if(VTKMY_WRAP_TCL)
    INCLUDE(${VTK_CMAKE_DIR}/vtkWrapTcl.cmake)
  endif()

else ()

  if (VTKMY_WRAP_TCL)
    message("Warning. VTKMY_WRAP_TCL is ON but the VTK version you have "
            "chosen has not support for Tcl (VTK_WRAP_TCL is OFF).  "
            "Please set VTKMY_WRAP_TCL to OFF.")
    set (VTKMY_WRAP_TCL OFF)
  endif ()

endif ()

#
# Python
#

if (VTK_WRAP_PYTHON)

  option(VTKMY_WRAP_PYTHON
         "Wrap classes into the Python interpreted language."
         ON)

  if (VTKMY_WRAP_PYTHON)
    set(VTK_WRAP_PYTHON_FIND_LIBS ON)
    include(${VTK_CMAKE_DIR}/vtkWrapPython.cmake)
    if (WIN32)
      if (NOT BUILD_SHARED_LIBS)
        message(FATAL_ERROR "Python support requires BUILD_SHARED_LIBS to be ON.")
        set (VTKMY_CAN_BUILD 0)
      endif ()
    endif ()
  endif ()

else ()

  if (VTKMY_WRAP_PYTHON)
    message("Warning. VTKMY_WRAP_PYTHON is ON but the VTK version you have "
            "chosen has not support for Python (VTK_WRAP_PYTHON is OFF).  "
            "Please set VTKMY_WRAP_PYTHON to OFF.")
    set (VTKMY_WRAP_PYTHON OFF)
  endif ()

endif ()

#
# Java
#

if (VTK_WRAP_JAVA)

  option(VTKMY_WRAP_JAVA
         "Wrap classes into the Java interpreted language."
         ON)

  if (VTKMY_WRAP_JAVA)
    set(VTK_WRAP_JAVA3_INIT_DIR "${VTKMY_SOURCE_DIR}/Wrapping")
    include(${VTK_CMAKE_DIR}/vtkWrapJava.cmake)
    if (WIN32)
      if (NOT BUILD_SHARED_LIBS)
        message(FATAL_ERROR "Java support requires BUILD_SHARED_LIBS to be ON.")
        set (VTKMY_CAN_BUILD 0)
      endif ()
    endif ()

    # Tell the java wrappers where to go.
    set(VTK_JAVA_HOME ${VTKMY_BINARY_DIR}/java/vtkmy)
    file(MAKE_DIRECTORY ${VTK_JAVA_HOME})
  endif ()

else ()

  if (VTKMY_WRAP_JAVA)
    message("Warning. VTKMY_WRAP_JAVA is ON but the VTK version you have "
            "chosen has not support for Java (VTK_WRAP_JAVA is OFF).  "
            "Please set VTKMY_WRAP_JAVA to OFF.")
    set (VTKMY_WRAP_JAVA OFF)
  endif ()

endif ()

# Setup our local hints file in case wrappers need them.
set(VTK_WRAP_HINTS ${VTKMY_SOURCE_DIR}/Wrapping/hints)
