SUBDIRS(
  Ex1
  Ex2
)

#
# Link to VTK and local
#

LINK_LIBRARIES (
  vtkmyUnsorted
  vtkIO
  vtkGraphics
  vtkmyImaging 
  vtkImaging 
  vtkFiltering
  vtkmyCommon
  vtkCommon
)

IF (VTK_USE_HYBRID)
  LINK_LIBRARIES (vtkHybrid)
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PARALLEL)
  LINK_LIBRARIES (vtkParallel)
ENDIF (VTK_USE_PARALLEL)

IF (VTK_USE_PATENTED)
  LINK_LIBRARIES (vtkPatented)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_RENDERING)
  LINK_LIBRARIES (vtkRendering)
ENDIF (VTK_USE_RENDERING)
