SOURCE_FILES( Graphics_SRCS
vtkAppendFilter
vtkAppendPolyData
vtkApproximatingSubdivisionFilter
vtkArrayCalculator
vtkAttributeDataToFieldDataFilter
vtkAxes
vtkBlankStructuredGridWithImage
vtkBrownianPoints
vtkButterflySubdivisionFilter
vtkCardinalSpline
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
vtkDataSetSurfaceFilter
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
vtkGaussianSplatter
vtkGeometryFilter
vtkGlyph2D
vtkGlyph3D
vtkGlyphSource2D
vtkHedgeHog
vtkHyperStreamline
vtkIdFilter
vtkImplicitModeller
vtkImplicitTextureCoords
vtkInterpolateDataSetAttributes
vtkInterpolatingSubdivisionFilter
vtkKochanekSpline
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
vtkMergePoints
vtkMergePoints2D 
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
vtkScalarTree
vtkSelectPolyData
vtkShepardMethod
vtkShrinkFilter
vtkShrinkPolyData
vtkSimpleElevationFilter
vtkSmoothPolyDataFilter
vtkSpatialRepresentationFilter
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
vtkSurfaceReconstructionFilter 
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
vtkVoxelModeller
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

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkGraphicsTCL GraphicsTCL_SRCS Graphics_SRCS)
  ADD_LIBRARY(vtkGraphicsTCL GraphicsTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkGraphicsTCL vtkFilteringTCL vtkCommonTCL 
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib vtkGraphicsTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkGraphicsPython GraphicsPython_SRCS Graphics_SRCS)
  ADD_LIBRARY(vtkGraphicsPython GraphicsPython_SRCS)
  TARGET_LINK_LIBRARIES (vtkGraphicsPython vtkFilteringPython vtkCommonPython )
  TARGET_LINK_LIBRARIES (vtkGraphicsPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
  INSTALL_TARGETS(/lib vtkGraphicsPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkGraphicsJava GraphicsJava_SRCS Graphics_SRCS)
  ADD_LIBRARY(vtkGraphicsJava GraphicsJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkGraphicsJava vtkCommonJava vtkFilteringJava)
  INSTALL_TARGETS(/lib vtkGraphicsJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering
vtkGraphics 
)


INSTALL_TARGETS(/lib vtkGraphics)
INSTALL_FILES(/include/vtk .h Graphics_SRCS)
