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

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkPatentedTCL PatentedTCL_SRCS Patented_SRCS)
  ADD_LIBRARY(vtkPatentedTCL PatentedTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkPatentedTCL vtkIOTCL 
                         vtkGraphicsTCL vtkImagingTCL vtkFilteringTCL 
                         vtkCommonTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  ADD_LIBRARY(vtkPatentedPython PatentedPython_SRCS)
  VTK_WRAP_PYTHON(vtkPatentedPython PatentedPython_SRCS Patented_SRCS)
  TARGET_LINK_LIBRARIES (vtkPatentedPython 
                         vtkGraphicsPython
                         vtkImagingPython
                         vtkFilteringPython
                         vtkCommonPython)
  TARGET_LINK_LIBRARIES (vtkPatentedPython 
                         debug python21_d optimized python21)
ENDIF (VTK_WRAP_PYTHON)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
vtkPatented
)


