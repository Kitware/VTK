#
# Source files
#
# Here is where you can add the name of your local unsorted classes.
#

SOURCE_FILES (Unsorted_SRCS
  vtkBar2
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

#
# Include path(s)
#

INCLUDE_DIRECTORIES(${VTKMY_BINARY_DIR}/Common)

INCLUDE_DIRECTORIES(${VTKMY_SOURCE_DIR}/Common)
INCLUDE_DIRECTORIES (${VTKMY_SOURCE_DIR}/Imaging)
INCLUDE_DIRECTORIES (${VTKMY_SOURCE_DIR}/Unsorted)

#
# Create the vtkmyUnsorted C++ library
#

ADD_LIBRARY (vtkmyUnsorted Unsorted_SRCS)

#
# Create the vtkmyUnsorted Tcl library
#

IF (VTKMY_WRAP_TCL)
  VTK_WRAP_TCL (vtkmyUnsortedTCL UnsortedTCL_SRCS Unsorted_SRCS)
  ADD_LIBRARY (vtkmyUnsortedTCL SHARED UnsortedTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkmyUnsortedTCL 
                         vtkHybridTCL 
                         vtkRenderingTCL 
                         vtkIOTCL 
                         vtkGraphicsTCL 
                         vtkmyImagingTCL
                         vtkImagingTCL 
                         vtkFilteringTCL 
                         vtkmyCommonTCL
                         vtkCommonTCL
                         ${TCL_LIBRARY})
ENDIF (VTKMY_WRAP_TCL)

#
# Create the vtkmyUnsorted Python library
#

IF (VTKMY_WRAP_PYTHON)
  VTK_WRAP_PYTHON (vtkmyUnsortedPython UnsortedPython_SRCS Unsorted_SRCS)
  IF (APPLE)
    ADD_LIBRARY (vtkmyUnsortedPython MODULE UnsortedPython_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY (vtkmyUnsortedPython SHARED UnsortedPython_SRCS)
    TARGET_LINK_LIBRARIES (vtkmyUnsortedPython 
                           vtkHybridPython 
                           vtkRenderingPython
                           vtkIOPython
                           vtkGraphicsPython
                           vtkmyImagingPython
                           vtkImagingPython
                           vtkFilteringPython
                           vtkmyCommonPython
                           vtkCommonPython)
  ENDIF (APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkmyUnsortedPython 
                           vtkHybridPython 
                           vtkRenderingPython
                           vtkIOPython
                           vtkGraphicsPython
                           vtkmyImagingPython
                           vtkImagingPython
                           vtkFilteringPython
                           vtkmyCommonPython
                           vtkCommonPython
                           debug ${PYTHON_DEBUG_LIBRARY} 
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtkmyUnsortedPython 
                           vtkHybridPython 
                           vtkRenderingPython
                           vtkIOPython
                           vtkGraphicsPython
                           vtkmyImagingPython
                           vtkImagingPython
                           vtkFilteringPython
                           vtkmyCommonPython
                           vtkCommonPython
                           ${PYTHON_LIBRARY})
  ENDIF(WIN32)
ENDIF (VTKMY_WRAP_PYTHON)

#
# Create the vtkmyUnsorted Java library
#

IF (VTKMY_WRAP_JAVA)
  VTK_WRAP_TCL (vtkmyUnsortedJava UnsortedJava_SRCS Unsorted_SRCS)
  ADD_LIBRARY (vtkmyUnsortedJava SHARED UnsortedJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkmyUnsortedJava 
                         vtkHybridJava 
                         vtkRenderingJava
                         vtkIOJava
                         vtkGraphicsJava
                         vtkmyImagingJava
                         vtkImagingJava 
                         vtkFilteringJava
                         vtkmyCommonJava
                         vtkCommonJava)
ENDIF (VTKMY_WRAP_JAVA)

#
# Link to VTK and local
#

LINK_LIBRARIES (
  vtkmyUnsorted
  vtkHybrid 
  vtkRendering
  vtkIO
  vtkGraphics
  vtkmyImaging 
  vtkImaging 
  vtkFiltering
  vtkmyCommon
  vtkCommon
)
