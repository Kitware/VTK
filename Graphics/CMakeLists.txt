SET(KIT Graphics)
SET(UKIT GRAPHICS)
SET(KIT_TCL_LIBS vtkFilteringTCL)
IF(APPLE)
  SET(KIT_PYTHON_LIBS vtkFilteringPythonD)
ELSE(APPLE)
  SET(KIT_PYTHON_LIBS vtkFilteringPython)
ENDIF(APPLE)
SET(KIT_JAVA_LIBS vtkFilteringJava)
SET(KIT_LIBS vtkFiltering)


SET( Kit_SRCS
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
vtkBoxClipDataSet.cxx
vtkBrownianPoints.cxx
vtkButterflySubdivisionFilter.cxx
vtkButtonSource.cxx
vtkBSPCuts.cxx
vtkBSPIntersections.cxx
vtkCellCenterDepthSort.cxx
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
vtkCurvatures.cxx
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
vtkDiscreteMarchingCubes.cxx
vtkDiskSource.cxx
vtkEdgePoints.cxx
vtkElevationFilter.cxx
vtkEllipticalButtonSource.cxx
vtkExtractCells.cxx
vtkExtractDataOverTime.cxx
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
vtkGridSynchronizedTemplates3D.cxx
vtkHierarchicalDataSetGeometryFilter.cxx
vtkHedgeHog.cxx
vtkHierarchicalDataExtractDataSets.cxx
vtkHierarchicalDataExtractLevel.cxx
vtkHierarchicalDataLevelFilter.cxx
vtkHull.cxx
vtkHyperStreamline.cxx
vtkIdFilter.cxx
vtkImageDataGeometryFilter.cxx
vtkImageMarchingCubes.cxx
vtkImplicitTextureCoords.cxx
vtkInterpolateDataSetAttributes.cxx
vtkInterpolatingSubdivisionFilter.cxx
vtkKdNode.cxx
vtkKdTree.cxx
vtkLineSource.cxx
vtkLinearExtrusionFilter.cxx
vtkLinearSubdivisionFilter.cxx
vtkLinkEdgels.cxx
vtkLoopSubdivisionFilter.cxx
vtkMarchingContourFilter.cxx
vtkMarchingCubes.cxx
vtkMarchingSquares.cxx
vtkMaskFields.cxx
vtkMaskPoints.cxx
vtkMaskPolyData.cxx
vtkMassProperties.cxx
vtkMergeDataObjectFilter.cxx
vtkMergeCells.cxx
vtkMergeFields.cxx
vtkMergeFilter.cxx
vtkMeshQuality.cxx
vtkModelMetadata.cxx
vtkOBBDicer.cxx
vtkOBBTree.cxx
vtkOutlineCornerFilter.cxx
vtkOutlineCornerSource.cxx
vtkOutlineFilter.cxx
vtkOutlineSource.cxx
vtkParametricFunctionSource.cxx
vtkPlaneSource.cxx
vtkPlanesIntersection.cxx
vtkPlatonicSolidSource.cxx
vtkPointDataToCellData.cxx
vtkPointsProjectedHull.cxx
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
vtkRectangularButtonSource.cxx
vtkRectilinearGridGeometryFilter.cxx
vtkRectilinearGridToTetrahedra.cxx
vtkRectilinearSynchronizedTemplates.cxx
vtkRecursiveDividingCubes.cxx
vtkReflectionFilter.cxx
vtkRegularPolygonSource.cxx
vtkReverseSense.cxx
vtkRibbonFilter.cxx
vtkRotationalExtrusionFilter.cxx
vtkRotationFilter.cxx
vtkRuledSurfaceFilter.cxx
vtkSelectPolyData.cxx
vtkShrinkFilter.cxx
vtkShrinkPolyData.cxx
vtkSimpleElevationFilter.cxx
vtkSliceCubes.cxx
vtkSmoothPolyDataFilter.cxx
vtkSortDataArray.cxx
vtkSpatialRepresentationFilter.cxx
vtkSpherePuzzle.cxx
vtkSpherePuzzleArrows.cxx
vtkSphereSource.cxx
vtkSplineFilter.cxx
vtkSplitField.cxx
vtkStreamLine.cxx
vtkStreamPoints.cxx
vtkStreamTracer.cxx
vtkStreamer.cxx
vtkStripper.cxx
vtkStructuredGridGeometryFilter.cxx
vtkStructuredGridOutlineFilter.cxx
vtkStructuredPointsGeometryFilter.cxx
vtkSubPixelPositionEdgels.cxx
vtkSubdivideTetra.cxx
vtkSuperquadricSource.cxx
vtkSynchronizedTemplates2D.cxx
vtkSynchronizedTemplates3D.cxx
vtkSynchronizedTemplatesCutter3D.cxx
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
vtkVisibilitySort.cxx
vtkVoxelContoursToSurfaceFilter.cxx
vtkWarpLens.cxx
vtkWarpScalar.cxx
vtkWarpTo.cxx
vtkWarpVector.cxx
vtkWindowedSincPolyDataFilter.cxx
)

SET_SOURCE_FILES_PROPERTIES(
vtkApproximatingSubdivisionFilter
vtkButtonSource
vtkDicer
vtkInterpolatingSubdivisionFilter
vtkStreamer
vtkVisibilitySort.cxx
ABSTRACT
)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

# -----------------------------------------------------------------
# Start of common section

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
                       "${Kit_SRCS}"
                       VTK_${UKIT}_EXPORT
                       ${VTK_BINARY_DIR} "")

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${Kit_EXTRA_SRCS} ${KitInstantiator_SRCS})

# Allow the user to customize their build with some local options
#
SET(LOCALUSERMACRODEFINED 0)
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL3(vtk${KIT}TCL KitTCL_SRCS 
                "${Kit_SRCS}"
                "${Kit_TCL_EXTRA_CMDS}")
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS} ${Kit_TCL_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} ${KIT_TCL_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}TCL)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  IF(KIT_TCL_DEPS)
    ADD_DEPENDENCIES(vtk${KIT}TCL ${KIT_TCL_DEPS})
  ENDIF(KIT_TCL_DEPS)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON3(vtk${KIT}Python KitPython_SRCS "${Kit_SRCS}")
  IF (APPLE)
    ADD_LIBRARY(vtk${KIT}PythonD ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    ADD_LIBRARY(vtk${KIT}Python MODULE vtk${KIT}PythonInit.cxx)
    TARGET_LINK_LIBRARIES(vtk${KIT}PythonD vtk${KIT} ${KIT_PYTHON_LIBS}
                          ${VTK_PYTHON_LIBRARIES})
    TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT}PythonD)
    IF(NOT VTK_INSTALL_NO_LIBRARIES)
      INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}PythonD)
    ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  ELSE (APPLE)
    ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    TARGET_LINK_LIBRARIES (vtk${KIT}Python vtk${KIT} ${KIT_PYTHON_LIBS})
  ENDIF(APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python ${VTK_PYTHON_LIBRARIES})
  ENDIF(WIN32)
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}Python)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  IF(KIT_PYTHON_DEPS)
    ADD_DEPENDENCIES(vtk${KIT}Python ${KIT_PYTHON_DEPS})
  ENDIF(KIT_PYTHON_DEPS)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA3(vtk${KIT}Java KitJava_SRCS "${Kit_SRCS}")
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(vtk${KIT}Java vtk${KIT} ${KIT_JAVA_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}Java)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  IF(KIT_JAVA_DEPS)
    ADD_DEPENDENCIES(vtk${KIT}TCL ${KIT_JAVA_DEPS})
  ENDIF(KIT_JAVA_DEPS)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES(vtk${KIT} ${KIT_LIBS})

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT})
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h ${Kit_SRCS})
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

VTK_EXPORT_KIT("${KIT}" "${UKIT}" "${Kit_SRCS}")

# If the user defined a custom macro, execute it now and pass in all the srcs
#
IF(LOCALUSERMACRODEFINED)
  LocalUserOptionsMacro( "${Kit_SRCS}"       "${Kit_EXTRA_SRCS}"
                         "${KitTCL_SRCS}"    "${Kit_TCL_EXTRA_SRCS}"
                         "${KitJava_SRCS}"   "${Kit_JAVA_EXTRA_SRCS}"
                         "${KitPython_SRCS}" "${Kit_PYTHON_EXTRA_SRCS}")
ENDIF(LOCALUSERMACRODEFINED)

# End of common section
# -----------------------------------------------------------------
