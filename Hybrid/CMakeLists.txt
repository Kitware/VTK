SOURCE_FILES ( Hybrid_SRCS
vtkArcPlotter 
vtkCubeAxesActor2D 
vtkDepthSortPolyData 
vtkEarthSource 
vtkGridTransform 
vtkImplicitModeller
vtkIVWriter 
vtkImageToPolyDataFilter 
vtkIterativeClosestPointTransform
vtkLandmarkTransform 
vtkLegendBoxActor 
vtkRenderLargeImage 
vtkRIBExporter 
vtkRIBLight 
vtkRIBProperty 
vtkThinPlateSplineTransform 
vtkTransformToGrid 
vtkVectorText
vtkVideoSource 
vtkVRMLImporter 
vtkXYPlotActor 
vtkWeightedTransformFilter 
)

WRAP_EXCLUDE_FILES(vtkVRML)

INCLUDE (${CMAKE_ROOT}/Modules/FindOpenGL.cmake)

ADD_LIBRARY(vtkHybrid Hybrid_SRCS)

# Allow the user to customize their build with some local options
#
INCLUDE (LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkHybridTCL HybridTCL_SRCS Hybrid_SRCS)
  ADD_LIBRARY(vtkHybridTCL HybridTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkHybridTCL 
                         vtkRenderingTCL 
                         vtkIOTCL 
                         vtkGraphicsTCL 
                         vtkImagingTCL 
                         vtkFilteringTCL 
                         vtkCommonTCL
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib vtkHybridTCL)
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
  INSTALL_TARGETS(/lib vtkHybridPython)
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
  INSTALL_TARGETS(/lib vtkHybridJava)
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


INSTALL_TARGETS(/lib vtkHybrid)
INSTALL_FILES(/include/vtk .h Hybrid_SRCS)
