SOURCE_FILES ( Hybrid_SRCS
vtkArcPlotter 
vtkBranchExtentTranslator 
vtkCellDerivatives 
vtkCubeAxesActor2D 
vtkDEMReader 
vtkDepthSortPolyData 
vtkEarthSource 
vtkGridTransform 
vtkIVWriter 
vtkImageToPolyDataFilter 
vtkImageReslice 
vtkLandmarkTransform 
vtkLegendBoxActor 
vtkLightKit 
vtkMassProperties 
vtkOBJReader 
vtkOutlineCornerFilter 
vtkOutlineCornerSource 
vtkProjectedTexture 
vtkRenderLargeImage 
vtkRIBExporter 
vtkRIBLight 
vtkRIBProperty 
vtkSuperquadric 
vtkSurfaceReconstructionFilter 
vtkSuperquadricSource 
vtkSubdivideTetra 
vtkThinPlateSplineTransform 
vtkTransformToGrid 
vtkVideoSource 
vtkVolumeProMapper 
vtkVRMLImporter 
vtkXYPlotActor 
vtkWeightedTransformFilter 
)

INCLUDE (${CMAKE_SOURCE_DIR}/CMake/Modules/FindOpenGL.cmake)

IF (WIN32)
  SOURCE_FILES(Hybrid_SRCS vtkWin32OffscreenRenderWindow)
  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
ENDIF (WIN32)

ADD_LIBRARY(vtkHybrid Hybrid_SRCS)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkHybridTCL HybridTCL_SRCS Hybrid_SRCS)
  ADD_LIBRARY(vtkHybridTCL HybridTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkHybridTCL vtkRenderingTCL vtkIOTCL vtkGraphicsTCL 
                         vtkImagingTCL vtkFilteringTCL vtkCommonTCL)
ENDIF (VTK_WRAP_TCL)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
vtkRendering
vtkHybrid
)


