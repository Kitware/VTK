#
# Try to find VTK and include its settings (otherwise complain)
#

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)

IF (USE_VTK_FILE)
  INCLUDE (${USE_VTK_FILE})
ELSE (USE_VTK_FILE)
  MESSAGE("Error, unable to find VTK, please edit your CMakeCache.txt file to specify the correct location of your VTK build")
  SET (VTKMY_CAN_BUILD 0)
ENDIF (USE_VTK_FILE)

#
# Output path(s)
#

SET (LIBRARY_OUTPUT_PATH ${VTKMY_BINARY_DIR}/bin/ CACHE PATH 
     "Single output directory for building all libraries.")

SET (EXECUTABLE_OUTPUT_PATH ${VTKMY_BINARY_DIR}/bin/ CACHE PATH 
     "Single output directory for building all executables.")

#
# Build shared libs ?
#
# Defaults to the same VTK setting.
#

OPTION(BUILD_SHARED_LIBS 
       "Build with shared libraries." 
       ${VTK_BUILD_SHARED_LIBS})

#
# Wrap Tcl, Java, Python
#
# Rational: even if your VTK was wrapped, it does not mean that you want to 
# wrap your own local classes. 
#

IF (VTK_WRAP_TCL)
  OPTION(VTKMY_WRAP_TCL 
         "Wrap classes into the TCL intepreted language." 
         ${VTK_WRAP_TCL})
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_JAVA)
  OPTION(VTKMY_WRAP_JAVA 
         "Wrap classes into the Java intepreted language." 
         ${VTK_WRAP_JAVA})
ENDIF (VTK_WRAP_JAVA)

IF (VTK_WRAP_PYTHON)
  OPTION(VTKMY_WRAP_PYTHON 
         "Wrap classes into the Python intepreted language." 
         ${VTK_WRAP_PYTHON})
ENDIF (VTK_WRAP_PYTHON)
