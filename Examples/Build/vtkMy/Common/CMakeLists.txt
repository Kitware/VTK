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
  VTK_WRAP_TCL (vtkmyCommonTCL CommonTCL_SRCS ${Common_SRCS})
  ADD_LIBRARY (vtkmyCommonTCL ${CommonTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkmyCommonTCL vtkmyCommon vtkCommonTCL)
ENDIF (VTK_WRAP_TCL AND VTKMY_WRAP_TCL)

# Create the vtkmyCommon Python library.
IF (VTK_WRAP_PYTHON AND VTKMY_WRAP_PYTHON)
  VTK_WRAP_PYTHON (vtkmyCommonPython CommonPython_SRCS ${Common_SRCS})
  ADD_LIBRARY (vtkmyCommonPython MODULE ${CommonPython_SRCS})
  TARGET_LINK_LIBRARIES (vtkmyCommonPython vtkmyCommon vtkCommonPython)
ENDIF (VTK_WRAP_PYTHON AND VTKMY_WRAP_PYTHON)

# Create the vtkmyCommon Java library.
IF (VTK_WRAP_JAVA AND VTKMY_WRAP_JAVA)
  VTK_WRAP_JAVA (vtkmyCommonJava CommonJava_SRCS ${Common_SRCS})
  ADD_LIBRARY (vtkmyCommonJava SHARED ${CommonJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkmyCommonJava vtkmyCommon vtkCommonJava)
ENDIF (VTK_WRAP_JAVA AND VTKMY_WRAP_JAVA)
