#
# Source files
#
# Here is where you can add the name of your local imaging classes.
#

SOURCE_FILES (Imaging_SRCS
  vtkImageFoo
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

#
# Include path(s)
#

INCLUDE_DIRECTORIES(${VTKMY_BINARY_DIR}/Common)

INCLUDE_DIRECTORIES(${VTKMY_SOURCE_DIR}/Common)
INCLUDE_DIRECTORIES (${VTKMY_SOURCE_DIR}/Imaging)

#
# Create the vtkmyImaging C++ library
#

ADD_LIBRARY (vtkmyImaging Imaging_SRCS)

#
# Create the vtkmyImaging Tcl library
#

IF (VTKMY_WRAP_TCL)
  VTK_WRAP_TCL (vtkmyImagingTCL ImagingTCL_SRCS Imaging_SRCS)
  ADD_LIBRARY (vtkmyImagingTCL ImagingTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkmyImagingTCL 
                         vtkImagingTCL 
                         vtkFilteringTCL 
                         vtkmyCommonTCL
                         vtkCommonTCL
                         ${TCL_LIBRARY})
ENDIF (VTKMY_WRAP_TCL)

#
# Create the vtkmyImaging Python library
#

IF (VTKMY_WRAP_PYTHON)
  VTK_WRAP_PYTHON (vtkmyImagingPython ImagingPython_SRCS Imaging_SRCS)
  IF (APPLE)
    ADD_LIBRARY (vtkmyImagingPython MODULE ImagingPython_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY (vtkmyImagingPython SHARED ImagingPython_SRCS)
    TARGET_LINK_LIBRARIES (vtkmyImagingPython 
                           vtkImagingPython
                           vtkFilteringPython
                           vtkmyCommonPython
                           vtkCommonPython)
  ENDIF (APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkmyImagingPython 
                           vtkImagingPython
                           vtkFilteringPython
                           vtkmyCommonPython
                           vtkCommonPython
                           debug ${PYTHON_DEBUG_LIBRARY} 
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtkmyImagingPython 
                           vtkImagingPython
                           vtkFilteringPython
                           vtkmyCommonPython
                           vtkCommonPython
                           ${PYTHON_LIBRARY})
  ENDIF(WIN32)
ENDIF (VTKMY_WRAP_PYTHON)

#
# Create the vtkmyImaging Java library
#

IF (VTKMY_WRAP_JAVA)
  VTK_WRAP_TCL (vtkmyImagingJava ImagingJava_SRCS Imaging_SRCS)
  ADD_LIBRARY (vtkmyImagingJava SHARED ImagingJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkmyImagingJava 
                         vtkImagingJava
                         vtkFilteringJava
                         vtkmyCommonJava
                         vtkCommonJava)
ENDIF (VTKMY_WRAP_JAVA)

#
# Link to VTK and local
#

LINK_LIBRARIES (
  vtkmyImaging
  vtkImaging 
  vtkFiltering
  vtkmyCommon
  vtkCommon
)
