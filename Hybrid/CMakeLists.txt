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
vtkVRML
vtkVRMLImporter 
vtkXYPlotActor 
vtkWeightedTransformFilter 
)

WRAP_EXCLUDE_FILES(vtkVRML)

INCLUDE (${CMAKE_SOURCE_DIR}/CMake/Modules/FindOpenGL.cmake)

IF (WIN32)
  SOURCE_FILES(Hybrid_SRCS vtkWin32OffscreenRenderWindow)
  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
ENDIF (WIN32)

ADD_LIBRARY(vtkHybrid Hybrid_SRCS)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkHybridTCL HybridTCL_SRCS Hybrid_SRCS)
  ADD_LIBRARY(vtkHybridTCL HybridTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkHybridTCL 
                         vtkRenderingTCL 
                         vtkIOTCL 
                         vtkGraphicsTCL 
                         vtkImagingTCL 
                         vtkFilteringTCL 
                         vtkCommonTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkHybridPython HybridPython_SRCS Hybrid_SRCS)
  ADD_LIBRARY(vtkHybridPython HybridPython_SRCS)
  TARGET_LINK_LIBRARIES (vtkHybridPython 
                         vtkRenderingPython
                         vtkIOPython
                         vtkGraphicsPython
                         vtkImagingPython
                         vtkFilteringPython
                         vtkCommonPython)
  TARGET_LINK_LIBRARIES (vtkHybridPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkHybridJava HybridJava_SRCS Hybrid_SRCS)
  ADD_LIBRARY(vtkHybridJava HybridJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkHybridJava 
                         vtkCommonJava 
                         vtkFilteringJava
                         vtkImagingJava 
                         vtkGraphicsJava 
                         vtkRenderingJava
                         vtkIOJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
vtkRendering
vtkHybrid
)


