#
# Source files
#
# Here is where you can add the name of your local imaging classes.
#

SET (Imaging_SRCS
  vtkImageFoo.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkmyImaging C++ library
ADD_LIBRARY (vtkmyImaging ${Imaging_SRCS})
TARGET_LINK_LIBRARIES(vtkmyImaging vtkmyCommon vtkImaging)

# Create the vtkmyImaging Tcl library
IF (VTK_WRAP_TCL AND VTKMY_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkmyImagingTCL ImagingTCL_SRCS "${Imaging_SRCS}" "")
  ADD_LIBRARY (vtkmyImagingTCL ${ImagingTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkmyImagingTCL vtkmyCommonTCL vtkmyImaging vtkImagingTCL)
ENDIF (VTK_WRAP_TCL AND VTKMY_WRAP_TCL)

# Create the vtkmyImaging Python library.
IF (VTK_WRAP_PYTHON AND VTKMY_WRAP_PYTHON)
  INCLUDE_DIRECTORIES("${PYTHON_INCLUDE_PATH}")
  VTK_WRAP_PYTHON3 (vtkmyImagingPython ImagingPython_SRCS "${Imaging_SRCS}")
  ADD_LIBRARY (vtkmyImagingPythonD ${ImagingPython_SRCS})
  ADD_LIBRARY (vtkmyImagingPython MODULE vtkmyImagingPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkmyImagingPythonD vtkmyImaging vtkmyCommonPythonD vtkImaging vtkImagingPythonD)
  TARGET_LINK_LIBRARIES(vtkmyImagingPython vtkmyImagingPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKMY_WRAP_PYTHON)

# Create the vtkmyImaging Java library.
IF (VTK_WRAP_JAVA AND VTKMY_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkmyImagingJava ImagingJava_SRCS "${Imaging_SRCS}")
  ADD_LIBRARY (vtkmyImagingJava SHARED ${ImagingJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkmyImagingJava vtkmyCommonJava vtkmyImaging vtkImagingJava)
ENDIF (VTK_WRAP_JAVA AND VTKMY_WRAP_JAVA)
