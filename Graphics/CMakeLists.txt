
SOURCE_FILES( Graphics_SRCS
vtkAppendFilter.cxx
vtkAppendPolyData.cxx
vtkApproximatingSubdivisionFilter.cxx
vtkArrayCalculator.cxx
vtkArrowSource.cxx
vtkAssignAttribute.cxx
vtkAttributeDataToFieldDataFilter.cxx
vtkAxes.cxx
vtkBandedPolyDataContourFilter.cxx
vtkBlankStructuredGrid.cxx
vtkBlankStructuredGridWithImage.cxx
vtkBrownianPoints.cxx
vtkButterflySubdivisionFilter.cxx
vtkButtonSource.cxx
vtkCellCenters.cxx
vtkCellDataToPointData.cxx
vtkCellDerivatives.cxx
vtkCleanPolyData.cxx
vtkClipDataSet.cxx
vtkClipPolyData.cxx
vtkClipVolume.cxx
vtkConeSource.cxx
vtkConnectivityFilter.cxx
vtkContourFilter.cxx
vtkContourGrid.cxx
vtkCubeSource.cxx
vtkCursor3D.cxx
vtkCutter.cxx
vtkCylinderSource.cxx
vtkDashedStreamLine.cxx
vtkDataObjectToDataSetFilter.cxx
vtkDataSetSurfaceFilter.cxx
vtkDataSetToDataObjectFilter.cxx
vtkDataSetTriangleFilter.cxx
vtkDecimatePro.cxx
vtkDelaunay2D.cxx
vtkDelaunay3D.cxx
vtkDicer.cxx
vtkDiskSource.cxx
vtkEdgePoints.cxx
vtkElevationFilter.cxx
vtkExtractEdges.cxx
vtkExtractGeometry.cxx
vtkExtractGrid.cxx
vtkExtractPolyDataGeometry.cxx
vtkExtractRectilinearGrid.cxx
vtkExtractTensorComponents.cxx
vtkExtractUnstructuredGrid.cxx
vtkExtractVectorComponents.cxx
vtkFeatureEdges.cxx
vtkFieldDataToAttributeDataFilter.cxx
vtkGeometryFilter.cxx
vtkGlyph2D.cxx
vtkGlyph3D.cxx
vtkGlyphSource2D.cxx
vtkGraphLayoutFilter.cxx
vtkHedgeHog.cxx
vtkHull.cxx
vtkHyperStreamline.cxx
vtkIdFilter.cxx
vtkImageDataGeometryFilter.cxx
vtkImplicitTextureCoords.cxx
vtkInterpolateDataSetAttributes.cxx
vtkInterpolatingSubdivisionFilter.cxx
vtkLineSource.cxx
vtkLinearExtrusionFilter.cxx
vtkLinearSubdivisionFilter.cxx
vtkLinkEdgels.cxx
vtkLoopSubdivisionFilter.cxx
vtkMaskFields.cxx
vtkMaskPoints.cxx
vtkMaskPolyData.cxx
vtkMassProperties.cxx
vtkMergeDataObjectFilter.cxx
vtkMergeFields.cxx
vtkMergeFilter.cxx
vtkOBBDicer.cxx
vtkOBBTree.cxx
vtkOutlineCornerFilter.cxx
vtkOutlineCornerSource.cxx
vtkOutlineFilter.cxx
vtkOutlineSource.cxx
vtkPlaneSource.cxx
vtkPointDataToCellData.cxx
vtkPointSource.cxx
vtkPolyDataConnectivityFilter.cxx
vtkPolyDataNormals.cxx
vtkPolyDataStreamer.cxx
vtkProbeFilter.cxx
vtkProgrammableAttributeDataFilter.cxx
vtkProgrammableDataObjectSource.cxx
vtkProgrammableFilter.cxx
vtkProgrammableGlyphFilter.cxx
vtkProgrammableSource.cxx
vtkProjectedTexture.cxx
vtkQuadricClustering.cxx
vtkQuadricDecimation.cxx
vtkQuantizePolyDataPoints.cxx
vtkRearrangeFields.cxx
vtkRectilinearGridGeometryFilter.cxx
vtkRecursiveDividingCubes.cxx
vtkReflectionFilter.cxx
vtkReverseSense.cxx
vtkRibbonFilter.cxx
vtkRotationalExtrusionFilter.cxx
vtkRuledSurfaceFilter.cxx
vtkSelectPolyData.cxx
vtkShrinkFilter.cxx
vtkShrinkPolyData.cxx
vtkSimpleElevationFilter.cxx
vtkSmoothPolyDataFilter.cxx
vtkSpatialRepresentationFilter.cxx
vtkSpherePuzzle.cxx
vtkSpherePuzzleArrows.cxx
vtkSphereSource.cxx
vtkSplitField.cxx
vtkStreamLine.cxx
vtkStreamPoints.cxx
vtkStreamer.cxx
vtkStreamTracer.cxx
vtkStripper.cxx
vtkStructuredGridGeometryFilter.cxx
vtkStructuredGridOutlineFilter.cxx
vtkStructuredPointsGeometryFilter.cxx
vtkSubPixelPositionEdgels.cxx
vtkSubdivideTetra.cxx
vtkSuperquadricSource.cxx
vtkTensorGlyph.cxx
vtkTextSource.cxx
vtkTextureMapToCylinder.cxx
vtkTextureMapToPlane.cxx
vtkTextureMapToSphere.cxx
vtkTexturedSphereSource.cxx
vtkThreshold.cxx
vtkThresholdPoints.cxx
vtkThresholdTextureCoords.cxx
vtkTransformFilter.cxx
vtkTransformPolyDataFilter.cxx
vtkTransformTextureCoords.cxx
vtkTriangleFilter.cxx
vtkTriangularTCoords.cxx
vtkTubeFilter.cxx
vtkVectorDot.cxx
vtkVectorNorm.cxx
vtkVoxelContoursToSurfaceFilter.cxx
vtkWarpLens.cxx
vtkWarpScalar.cxx
vtkWarpTo.cxx
vtkWarpVector.cxx
vtkWindowedSincPolyDataFilter.cxx
)

ABSTRACT_FILES(
vtkApproximatingSubdivisionFilter
vtkDicer
vtkInterpolatingSubdivisionFilter
vtkStreamer
)

IF(VTK_MAKE_INSTANTIATOR)
  # Setup vtkInstantiator registration for this library's classes.
  VTK_MAKE_INSTANTIATOR(vtkGraphicsInstantiator GraphicsInstantiator_SRCS
                        Graphics_SRCS
                        EXPORT_MACRO VTK_GRAPHICS_EXPORT
                        HEADER_LOCATION ${VTK_BINARY_DIR})
  ADD_LIBRARY(vtkGraphics Graphics_SRCS GraphicsInstantiator_SRCS)
  INSTALL_FILES(/include/vtk .h ${VTK_BINARY_DIR}/vtkGraphicsInstantiator)
ELSE(VTK_MAKE_INSTANTIATOR)
  ADD_LIBRARY(vtkGraphics Graphics_SRCS)
ENDIF(VTK_MAKE_INSTANTIATOR)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Graphics/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Graphics/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkGraphicsTCL GraphicsTCL_SRCS Graphics_SRCS)
  IF (APPLE)
   ADD_LIBRARY(vtkGraphicsTCL SHARED GraphicsTCL_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY(vtkGraphicsTCL GraphicsTCL_SRCS)
  ENDIF (APPLE)
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
  ENDIF (APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkGraphicsPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtkGraphicsPython ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  INSTALL_TARGETS(/lib/vtk vtkGraphicsPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkGraphicsJava GraphicsJava_SRCS Graphics_SRCS)
  ADD_LIBRARY(vtkGraphicsJava SHARED GraphicsJava_SRCS)
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

