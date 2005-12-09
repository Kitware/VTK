#
# Source files
#
# Here is where you can add the name of your local unsorted classes.
#

SET (Unsorted_SRCS
  vtkBar2.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkmyUnsorted C++ library.
ADD_LIBRARY (vtkmyUnsorted ${Unsorted_SRCS})
TARGET_LINK_LIBRARIES(vtkmyUnsorted vtkmyImaging vtkGraphics vtkIO)

IF (VTK_USE_PARALLEL)
  TARGET_LINK_LIBRARIES (vtkmyUnsorted vtkParallel)
ENDIF (VTK_USE_PARALLEL)

IF (VTK_USE_RENDERING)
  TARGET_LINK_LIBRARIES (vtkmyUnsorted vtkRendering)
  TARGET_LINK_LIBRARIES (vtkmyUnsorted vtkVolumeRendering)
  TARGET_LINK_LIBRARIES (vtkmyUnsorted vtkHybrid)
  TARGET_LINK_LIBRARIES (vtkmyUnsorted vtkWidgets)
ENDIF (VTK_USE_RENDERING)

# Create the vtkmyUnsorted Tcl library.
IF (VTK_WRAP_TCL AND VTKMY_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkmyUnsortedTCL UnsortedTCL_SRCS "${Unsorted_SRCS}" "")
  ADD_LIBRARY (vtkmyUnsortedTCL ${UnsortedTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkmyUnsortedTCL vtkmyCommonTCL vtkmyUnsorted)
ENDIF (VTK_WRAP_TCL AND VTKMY_WRAP_TCL)

# Create the vtkmyUnsorted Python library.
IF (VTK_WRAP_PYTHON AND VTKMY_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkmyUnsortedPython UnsortedPython_SRCS "${Unsorted_SRCS}")
  ADD_LIBRARY (vtkmyUnsortedPythonD ${UnsortedPython_SRCS})
  ADD_LIBRARY (vtkmyUnsortedPython MODULE vtkmyUnsortedPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkmyUnsortedPythonD vtkmyUnsorted vtkmyCommonPythonD)
  TARGET_LINK_LIBRARIES(vtkmyUnsortedPython vtkmyUnsortedPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKMY_WRAP_PYTHON)

# Create the vtkmyUnsorted Java library.
IF (VTK_WRAP_JAVA AND VTKMY_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkmyUnsortedJava UnsortedJava_SRCS "${Unsorted_SRCS}")
  ADD_LIBRARY (vtkmyUnsortedJava SHARED ${UnsortedJava_SRCS})
  TARGET_LINK_LIBRARIES(vtkmyUnsortedJava vtkmyCommonJava vtkmyUnsorted)
ENDIF (VTK_WRAP_JAVA AND VTKMY_WRAP_JAVA)
