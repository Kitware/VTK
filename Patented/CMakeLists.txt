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
  TARGET_LINK_LIBRARIES (vtkPatentedTCL vtkRenderingTCL vtkIOTCL 
                         vtkGraphicsTCL vtkImagingTCL vtkFilteringTCL 
                         vtkCommonTCL)
ENDIF (VTK_WRAP_TCL)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
vtkRendering
vtkPatented
)


