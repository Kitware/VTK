
SOURCE_FILES( Graphics_SRCS
vtkAppendFilter
vtkAppendPolyData
vtkApproximatingSubdivisionFilter
vtkArrayCalculator
vtkAttributeDataToFieldDataFilter
vtkAxes
vtkBlankStructuredGrid
vtkBlankStructuredGridWithImage
vtkBrownianPoints
vtkButterflySubdivisionFilter
vtkCellCenters
vtkCellDataToPointData
vtkCellDerivatives 
vtkCleanPolyData
vtkClipDataSet
vtkClipPolyData
vtkClipVolume
vtkConeSource
vtkConnectivityFilter
vtkContourFilter
vtkContourGrid
vtkCubeSource
vtkCursor3D
vtkCutter
vtkCylinderSource
vtkDashedStreamLine
vtkDataObjectToDataSetFilter
vtkDataSetSurfaceFilter
vtkDataSetToDataObjectFilter
vtkDataSetTriangleFilter
vtkDecimatePro
vtkDelaunay2D
vtkDelaunay3D
vtkDicer
vtkDiskSource
vtkEdgePoints
vtkElevationFilter
vtkExtractEdges
vtkExtractGeometry
vtkExtractGrid
vtkExtractPolyDataGeometry
vtkExtractTensorComponents
vtkExtractUnstructuredGrid
vtkExtractVectorComponents
vtkFeatureEdges
vtkFieldDataToAttributeDataFilter
vtkGeometryFilter
vtkGlyph2D
vtkGlyph3D
vtkGlyphSource2D
vtkGraphLayoutFilter
vtkHedgeHog
vtkHull
vtkHyperStreamline
vtkIdFilter
vtkImplicitTextureCoords
vtkInterpolateDataSetAttributes
vtkInterpolatingSubdivisionFilter
vtkLineSource
vtkLinearExtrusionFilter
vtkLinearSubdivisionFilter
vtkLinkEdgels
vtkLoopSubdivisionFilter
vtkMaskPoints
vtkMaskPolyData
vtkMassProperties 
vtkMergeDataObjectFilter
vtkMergeFilter
vtkOBBDicer
vtkOBBTree
vtkOutlineCornerFilter 
vtkOutlineCornerSource 
vtkOutlineFilter
vtkOutlineSource
vtkPlaneSource
vtkPointDataToCellData
vtkPointSource
vtkPolyDataConnectivityFilter
vtkPolyDataNormals
vtkPolyDataStreamer
vtkProbeFilter
vtkProgrammableAttributeDataFilter
vtkProgrammableDataObjectSource
vtkProgrammableFilter
vtkProgrammableGlyphFilter
vtkProgrammableSource
vtkProjectedTexture 
vtkQuadricClustering
vtkQuadricDecimation
vtkQuantizePolyDataPoints
vtkRectilinearGridGeometryFilter
vtkRecursiveDividingCubes
vtkReverseSense
vtkRibbonFilter
vtkRotationalExtrusionFilter
vtkRuledSurfaceFilter
vtkSelectPolyData
vtkShrinkFilter
vtkShrinkPolyData
vtkSimpleElevationFilter
vtkSmoothPolyDataFilter
vtkSpatialRepresentationFilter
vtkSphereSource
vtkStreamLine
vtkStreamPoints
vtkStreamer
vtkStripper
vtkStructuredGridGeometryFilter
vtkStructuredGridOutlineFilter
vtkStructuredPointsGeometryFilter
vtkSubPixelPositionEdgels
vtkSubdivideTetra 
vtkSuperquadricSource 
vtkTensorGlyph
vtkTextSource
vtkTextureMapToCylinder
vtkTextureMapToPlane
vtkTextureMapToSphere
vtkTexturedSphereSource
vtkThreshold
vtkThresholdPoints
vtkThresholdTextureCoords
vtkTransformFilter
vtkTransformPolyDataFilter
vtkTransformTextureCoords
vtkTriangleFilter
vtkTriangularTCoords
vtkTubeFilter
vtkVectorDot
vtkVectorNorm
vtkVoxelContoursToSurfaceFilter
vtkWarpLens
vtkWarpScalar
vtkWarpTo
vtkWarpVector
vtkWindowedSincPolyDataFilter
)

ABSTRACT_FILES(
vtkApproximatingSubdivisionFilter
vtkDicer
vtkInterpolatingSubdivisionFilter
)

ADD_LIBRARY(vtkGraphics Graphics_SRCS)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Graphics/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Graphics/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkGraphicsTCL GraphicsTCL_SRCS Graphics_SRCS)
  ADD_LIBRARY(vtkGraphicsTCL GraphicsTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkGraphicsTCL vtkFilteringTCL vtkCommonTCL 
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkGraphicsTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkGraphicsPython GraphicsPython_SRCS Graphics_SRCS)
  IF (APPLE)
    ADD_LIBRARY(vtkGraphicsPython MODULE GraphicsPython_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY(vtkGraphicsPython SHARED GraphicsPython_SRCS)
    TARGET_LINK_LIBRARIES (vtkGraphicsPython vtkFilteringPython vtkCommonPython )
    TARGET_LINK_LIBRARIES (vtkGraphicsPython 
                           debug ${PYTHON_DEBUG_LIBRARY} 
                           optimized ${PYTHON_LIBRARY})
  ENDIF (APPLE)
  INSTALL_TARGETS(/lib/vtk vtkGraphicsPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkGraphicsJava GraphicsJava_SRCS Graphics_SRCS)
  ADD_LIBRARY(vtkGraphicsJava GraphicsJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkGraphicsJava vtkCommonJava vtkFilteringJava)
  INSTALL_TARGETS(/lib/vtk vtkGraphicsJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering
vtkGraphics 
)


INSTALL_TARGETS(/lib/vtk vtkGraphics)
INSTALL_FILES(/include/vtk .h Graphics_SRCS)

