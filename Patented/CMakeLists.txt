SOURCE_FILES ( Patented_SRCS
vtkDecimate 
vtkDividingCubes 
vtkGridSynchronizedTemplates3D 
vtkImageMarchingCubes 
vtkKitwareContourFilter 
vtkMarchingContourFilter 
vtkMarchingCubes 
vtkMarchingSquares 
vtkSliceCubes 
vtkSweptSurface 
vtkSynchronizedTemplates2D 
vtkSynchronizedTemplates3D
)

ADD_LIBRARY(vtkPatented Patented_SRCS)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Patented/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Patented/LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkPatentedTCL PatentedTCL_SRCS Patented_SRCS)
  ADD_LIBRARY(vtkPatentedTCL PatentedTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkPatentedTCL vtkIOTCL 
                         vtkGraphicsTCL vtkImagingTCL vtkFilteringTCL 
                         vtkCommonTCL
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib vtkPatentedTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  IF (APPLE)
    ADD_LIBRARY(vtkPatentedPython MODULE PatentedPython_SRCS)
  ELSE
    ADD_LIBRARY(vtkPatentedPython SHARED PatentedPython_SRCS)
  ENDIF
  VTK_WRAP_PYTHON(vtkPatentedPython PatentedPython_SRCS Patented_SRCS)
  TARGET_LINK_LIBRARIES (vtkPatentedPython 
                         vtkGraphicsPython
                         vtkImagingPython
                         vtkFilteringPython
                         vtkCommonPython)
  TARGET_LINK_LIBRARIES (vtkPatentedPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
  INSTALL_TARGETS(/lib vtkPatentedPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkPatentedJava PatentedJava_SRCS Patented_SRCS)
  ADD_LIBRARY(vtkPatentedJava PatentedJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkPatentedJava vtkCommonJava vtkFilteringJava
                         vtkImagingJava vtkGraphicsJava)
  INSTALL_TARGETS(/lib vtkPatentedJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
vtkPatented
)


INSTALL_TARGETS(/lib vtkPatented)
INSTALL_FILES(/include/vtk .h Patented_SRCS)
