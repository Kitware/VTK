#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SOURCE_FILES (Common_SRCS
  vtkBar
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

#
# Create the vtkmyCommon C++ library
#

ADD_LIBRARY (vtkmyCommon Common_SRCS)

#
# Create the vtkmyCommon Tcl library
#

IF (VTKMY_WRAP_TCL)

  VTK_WRAP_TCL (vtkmyCommonTCL CommonTCL_SRCS Common_SRCS)
  ADD_LIBRARY (vtkmyCommonTCL CommonTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkmyCommonTCL 
                         vtkCommonTCL 
                         ${TCL_LIBRARY})
ENDIF (VTKMY_WRAP_TCL)

#
# Create the vtkmyCommon Python library
#

IF (VTKMY_WRAP_PYTHON)

  VTK_WRAP_PYTHON (vtkmyCommonPython CommonPython_SRCS Common_SRCS)

  IF (APPLE)
    ADD_LIBRARY (vtkmyCommonPython MODULE CommonPython_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY (vtkmyCommonPython SHARED CommonPython_SRCS)
    TARGET_LINK_LIBRARIES (vtkmyCommonPython 
	                   vtkCommonPython)
  ENDIF (APPLE)

  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkmyCommonPython 
	                   vtkCommonPython
                           debug ${PYTHON_DEBUG_LIBRARY} 
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtkmyCommonPython 
	                   vtkCommonPython
                           ${PYTHON_LIBRARY})
  ENDIF(WIN32)

ENDIF (VTKMY_WRAP_PYTHON)

#
# Create the vtkmyCommon Java library
#

IF (VTKMY_WRAP_JAVA)

  VTK_WRAP_TCL (vtkmyCommonJava CommonJava_SRCS Common_SRCS)
  ADD_LIBRARY (vtkmyCommonJava SHARED CommonJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkmyCommonJava 
                         vtkCommonJava)
ENDIF (VTKMY_WRAP_JAVA)

#
# Link to VTK and local
#

LINK_LIBRARIES (
  vtkmyCommon
  vtkCommon 
)

#
# Common configuration settings
#
# Do not worry about this one.
#

CONFIGURE_FILE(
  ${VTKMY_SOURCE_DIR}/Common/vtkmyConfigure.h.in 
  ${VTKMY_BINARY_DIR}/Common/vtkmyConfigure.h
)
