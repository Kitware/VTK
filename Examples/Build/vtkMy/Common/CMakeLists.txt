#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (Common_SRCS
  vtkBar.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkmyCommon C++ library.
ADD_LIBRARY (vtkmyCommon ${Common_SRCS})
TARGET_LINK_LIBRARIES (vtkmyCommon vtkCommon)

# Create the vtkmyCommon Tcl library.
IF (VTK_WRAP_TCL AND VTKMY_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkmyCommonTCL CommonTCL_SRCS "${Common_SRCS}" "")
  ADD_LIBRARY (vtkmyCommonTCL ${CommonTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkmyCommonTCL vtkmyCommon vtkCommonTCL)
ENDIF (VTK_WRAP_TCL AND VTKMY_WRAP_TCL)

# Create the vtkmyCommon Python library.
IF (VTK_WRAP_PYTHON AND VTKMY_WRAP_PYTHON)
  INCLUDE_DIRECTORIES("${PYTHON_INCLUDE_PATH}")
  VTK_WRAP_PYTHON3 (vtkmyCommonPython CommonPython_SRCS "${Common_SRCS}")
  ADD_LIBRARY (vtkmyCommonPythonD ${CommonPython_SRCS})
  ADD_LIBRARY (vtkmyCommonPython MODULE vtkmyCommonPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkmyCommonPythonD vtkmyCommon vtkCommon vtkCommonPythonD)
  TARGET_LINK_LIBRARIES(vtkmyCommonPython vtkmyCommonPythonD)
  SET_TARGET_PROPERTIES(vtkmyCommonPython PROPERTIES PREFIX "")
  IF(WIN32 AND NOT CYGWIN)
    SET_TARGET_PROPERTIES(vtkmyCommonPython PROPERTIES SUFFIX ".pyd")
  ENDIF(WIN32 AND NOT CYGWIN)
ENDIF (VTK_WRAP_PYTHON AND VTKMY_WRAP_PYTHON)

# Create the vtkmyCommon Java library.
IF (VTK_WRAP_JAVA AND VTKMY_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkmyCommonJava CommonJava_SRCS "${Common_SRCS}")
  ADD_LIBRARY (vtkmyCommonJava SHARED ${CommonJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkmyCommonJava vtkmyCommon vtkCommonJava)
ENDIF (VTK_WRAP_JAVA AND VTKMY_WRAP_JAVA)
