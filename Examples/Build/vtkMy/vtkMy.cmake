#
# Output path(s)
#

SET (LIBRARY_OUTPUT_PATH ${VTKMY_BINARY_DIR}/bin/ CACHE PATH 
     "Single output directory for building all libraries.")

SET (EXECUTABLE_OUTPUT_PATH ${VTKMY_BINARY_DIR}/bin/ CACHE PATH 
     "Single output directory for building all executables.")

MARK_AS_ADVANCED (
  LIBRARY_OUTPUT_PATH 
  EXECUTABLE_OUTPUT_PATH
)

#
# Try to find VTK and include its settings (otherwise complain)
#

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)

IF (USE_VTK_FILE)
  INCLUDE (${USE_VTK_FILE})
ELSE (USE_VTK_FILE)
  SET (VTKMY_CAN_BUILD 0)
ENDIF (USE_VTK_FILE)

#
# Build shared libs ?
#
# Defaults to the same VTK setting.
#

IF (USE_VTK_FILE)

  OPTION(BUILD_SHARED_LIBS 
         "Build with shared libraries." 
         ${VTK_BUILD_SHARED_LIBS})

ENDIF (USE_VTK_FILE)

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

IF (VTK_WRAP_TCL)

  OPTION(VTKMY_WRAP_TCL 
         "Wrap classes into the TCL interpreted language." 
         OFF)

  IF (VTKMY_WRAP_TCL)

    IF (NOT VTK_WRAP_TCL_EXE)

      MESSAGE("Error. Unable to find VTK_WRAP_TCL_EXE, please edit this value to specify the correct location of the VTK Tcl wrapper.")
      MARK_AS_ADVANCED(CLEAR VTK_WRAP_TCL_EXE)
      SET (VTKMY_CAN_BUILD 0)

    ELSE (NOT VTK_WRAP_TCL_EXE)

      FIND_FILE (VTK_WRAP_HINTS hints ${VTKMY_SOURCE_DIR}/Wrapping)
      MARK_AS_ADVANCED(VTK_WRAP_HINTS)

      IF (USE_INSTALLED_VTK)
        INCLUDE (${CMAKE_ROOT}/Modules/FindTCL.cmake)
      ENDIF (USE_INSTALLED_VTK)

      IF (TCL_INCLUDE_PATH)
        INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
      ENDIF (TCL_INCLUDE_PATH)

    ENDIF (NOT VTK_WRAP_TCL_EXE)
  ENDIF (VTKMY_WRAP_TCL)
ENDIF (VTK_WRAP_TCL)

#
# Python
# 

IF (VTK_WRAP_PYTHON)

  OPTION(VTKMY_WRAP_PYTHON 
         "Wrap classes into the Python interpreted language." 
         OFF)

  IF (VTKMY_WRAP_PYTHON)

    IF (NOT VTK_WRAP_PYTHON_EXE)

      MESSAGE("Error. Unable to find VTK_WRAP_PYTHON_EXE, please edit this value to specify the correct location of the VTK Python wrapper.")
      MARK_AS_ADVANCED(CLEAR VTK_WRAP_PYTHON_EXE)
      SET (VTKMY_CAN_BUILD 0)

    ELSE (NOT VTK_WRAP_PYTHON_EXE)

      FIND_FILE(VTK_WRAP_HINTS hints ${VTKMY_SOURCE_DIR}/Wrapping )
      MARK_AS_ADVANCED(VTK_WRAP_HINTS)

      IF (USE_INSTALLED_VTK)
        INCLUDE (${CMAKE_ROOT}/Modules/FindPythonLibs.cmake)
      ENDIF (USE_INSTALLED_VTK)

      IF (PYTHON_INCLUDE_PATH)
        INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
      ENDIF (PYTHON_INCLUDE_PATH)

      IF (WIN32)
        IF (NOT BUILD_SHARED_LIBS)
          MESSAGE("Error. Python support requires BUILD_SHARED_LIBS to be ON.")
          SET (VTKMY_CAN_BUILD 0)
        ENDIF (NOT BUILD_SHARED_LIBS)  
      ENDIF (WIN32)

    ENDIF (NOT VTK_WRAP_PYTHON_EXE)
  ENDIF (VTKMY_WRAP_PYTHON)
ENDIF (VTK_WRAP_PYTHON)

#
# Java
# 

IF (VTK_WRAP_JAVA)

  OPTION(VTKMY_WRAP_JAVA 
         "Wrap classes into the Java interpreted language." 
         OFF)

  IF (VTKMY_WRAP_JAVA)

    IF (NOT VTK_WRAP_JAVA_EXE)
      MESSAGE("Error. Unable to find VTK_WRAP_JAVA_EXE, please edit this value to specify the correct location of the VTK Java wrapper.")
      MARK_AS_ADVANCED(CLEAR VTK_WRAP_JAVA_EXE)
      SET (VTKMY_CAN_BUILD 0)
    ENDIF (NOT VTK_WRAP_JAVA_EXE)

    IF (NOT VTK_PARSE_JAVA_EXE)
      MESSAGE("Error. Unable to find VTK_PARSE_JAVA_EXE, please edit this value to specify the correct location of the VTK Java parser.")
      MARK_AS_ADVANCED(CLEAR VTK_PARSE_JAVA_EXE)
      SET (VTKMY_CAN_BUILD 0)
    ENDIF (NOT VTK_PARSE_JAVA_EXE)

    IF (VTK_WRAP_JAVA_EXE AND VTK_PARSE_JAVA_EXE)

      FIND_FILE(VTK_WRAP_HINTS hints ${VTKMY_SOURCE_DIR}/Wrapping )
      MARK_AS_ADVANCED(VTK_WRAP_HINTS)

      IF (USE_INSTALLED_VTK)
        INCLUDE (${CMAKE_ROOT}/Modules/FindJNI.cmake)
      ENDIF (USE_INSTALLED_VTK)

      IF (JAVA_INCLUDE_PATH)
        INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH})
      ENDIF (JAVA_INCLUDE_PATH)

      IF (JAVA_INCLUDE_PATH2)
        INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH2})
      ENDIF (JAVA_INCLUDE_PATH2)

      IF (JAVA_AWT_INCLUDE_PATH)
        INCLUDE_DIRECTORIES(${JAVA_AWT_INCLUDE_PATH})
      ENDIF (JAVA_AWT_INCLUDE_PATH)

      IF (NOT VTK_JAVA_HOME)
        SET (VTK_JAVA_HOME ${VTKMY_BINARY_DIR}/java/vtk CACHE PATH "Path to Java install")
      ENDIF (NOT VTK_JAVA_HOME)

      IF (WIN32)
        IF (NOT BUILD_SHARED_LIBS)
          MESSAGE("Error. Java support requires BUILD_SHARED_LIBS to be ON.")
          SET (VTKMY_CAN_BUILD 0)
        ENDIF (NOT BUILD_SHARED_LIBS)  
      ENDIF (WIN32)

    ENDIF (VTK_WRAP_JAVA_EXE AND VTK_PARSE_JAVA_EXE)
  ENDIF (VTKMY_WRAP_JAVA)
ENDIF (VTK_WRAP_JAVA)

