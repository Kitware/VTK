SET(KIT Filtering)
SET(UKIT FILTERING)
SET(KIT_TCL_LIBS vtkCommonTCL)
IF(APPLE)
  SET(KIT_PYTHON_LIBS vtkCommonPythonD)
ELSE(APPLE)
  SET(KIT_PYTHON_LIBS vtkCommonPython)
ENDIF(APPLE)
SET(KIT_JAVA_LIBS vtkCommonJava)
SET(KIT_LIBS vtkCommon)

SET( Kit_SRCS
vtkAbstractMapper.cxx
vtkActor2D.cxx
vtkActor2DCollection.cxx
vtkAlgorithm.cxx
vtkAlgorithmOutput.cxx
vtkCachedStreamingDemandDrivenPipeline.cxx
vtkCardinalSpline.cxx
vtkCastToConcrete.cxx
vtkCell.cxx
vtkCell3D.cxx
vtkCellArray.cxx
vtkCellData.cxx
vtkCellLinks.cxx
vtkCellLocator.cxx
vtkCellTypes.cxx
vtkColorTransferFunction.cxx
vtkCompositeDataIterator.cxx
vtkCompositeDataPipeline.cxx
vtkCompositeDataSet.cxx
vtkCone.cxx
vtkConvexPointSet.cxx
vtkCoordinate.cxx
vtkCylinder.cxx
vtkDataObject.cxx
vtkDataObjectCollection.cxx
vtkDataObjectSource.cxx
vtkDataSet.cxx
vtkDataSetAlgorithm.cxx
vtkDataSetAttributes.cxx
vtkDataSetCollection.cxx
vtkDataSetSource.cxx
vtkDataSetToDataSetFilter.cxx
vtkDataSetToImageFilter.cxx
vtkDataSetToPolyDataFilter.cxx
vtkDataSetToStructuredGridFilter.cxx
vtkDataSetToStructuredPointsFilter.cxx
vtkDataSetToUnstructuredGridFilter.cxx
vtkDemandDrivenPipeline.cxx
vtkEmptyCell.cxx
vtkExecutive.cxx
vtkExplicitCell.cxx
vtkFieldData.cxx
vtkFilteringInformationKeyManager.cxx
vtkGenericAdaptorCell.cxx
vtkGenericAttribute.cxx
vtkGenericAttributeCollection.cxx
vtkGenericCell.cxx
vtkGenericCellIterator.cxx
vtkGenericCellTessellator.cxx
vtkSimpleCellTessellator.cxx
vtkGenericDataSet.cxx
vtkGenericDataSetSource.cxx
vtkGenericDataSetToPolyDataFilter.cxx
vtkGenericDataSetToUnstructuredGridFilter.cxx
vtkGenericEdgeTable.cxx
vtkGenericPointIterator.cxx
vtkGenericSubdivisionErrorMetric.cxx
vtkGeometricErrorMetric.cxx
vtkAttributesErrorMetric.cxx
vtkHexagonalPrism.cxx
vtkHexahedron.cxx
vtkHierarchicalBoxDataSet.cxx
vtkHierarchicalDataInformation.cxx
vtkHierarchicalDataIterator.cxx
vtkHierarchicalDataSet.cxx
vtkHierarchicalDataSetAlgorithm.cxx
vtkImageAlgorithm.cxx
vtkImageData.cxx
vtkImageInPlaceFilter.cxx
vtkImageIterator.cxx
vtkImageMultipleInputFilter.cxx
vtkImageMultipleInputOutputFilter.cxx
vtkImageProgressIterator.cxx
vtkImageSource.cxx
vtkImageToImageFilter.cxx
vtkImageToStructuredPoints.cxx
vtkImageTwoInputFilter.cxx
vtkImplicitBoolean.cxx
vtkImplicitDataSet.cxx
vtkImplicitSelectionLoop.cxx
vtkImplicitSum.cxx
vtkImplicitVolume.cxx
vtkImplicitWindowFunction.cxx
vtkInformation.cxx
vtkInformationDataObjectKey.cxx
vtkInformationDoubleKey.cxx
vtkInformationDoubleVectorKey.cxx
vtkInformationExecutivePortKey.cxx
vtkInformationExecutivePortVectorKey.cxx
vtkInformationInformationKey.cxx
vtkInformationInformationVectorKey.cxx
vtkInformationIntegerKey.cxx
vtkInformationIntegerVectorKey.cxx
vtkInformationKey.cxx
vtkInformationKeyVectorKey.cxx
vtkInformationObjectBaseKey.cxx
vtkInformationStringKey.cxx
vtkInformationUnsignedLongKey.cxx
vtkInformationVector.cxx
vtkInterpolatedVelocityField.cxx
vtkKochanekSpline.cxx
vtkLine.cxx
vtkLocator.cxx
vtkMapper2D.cxx
vtkMergePoints.cxx
vtkMultiBlockDataSet.cxx
vtkMultiBlockDataSetAlgorithm.cxx
vtkMultiBlockDataIterator.cxx
vtkNonLinearCell.cxx
vtkOrderedTriangulator.cxx
vtkPentagonalPrism.cxx
vtkPerlinNoise.cxx
vtkPiecewiseFunction.cxx
vtkPiecewiseFunctionShiftScale.cxx
vtkPiecewiseFunctionSource.cxx
vtkPiecewiseFunctionToPiecewiseFunctionFilter.cxx
vtkPixel.cxx
vtkPointData.cxx
vtkPointLocator.cxx
vtkPointSet.cxx
vtkPointSetAlgorithm.cxx
vtkPointSetSource.cxx
vtkPointSetToPointSetFilter.cxx
vtkPolyData.cxx
vtkPolyDataAlgorithm.cxx
vtkPolyDataCollection.cxx
vtkPolyDataSource.cxx
vtkPolyDataToPolyDataFilter.cxx
vtkPolyLine.cxx
vtkPolyVertex.cxx
vtkPolygon.cxx
vtkProcessObject.cxx
vtkPropAssembly.cxx
vtkPyramid.cxx
vtkQuad.cxx
vtkQuadraticEdge.cxx
vtkQuadraticHexahedron.cxx
vtkQuadraticPyramid.cxx
vtkQuadraticQuad.cxx
vtkQuadraticTetra.cxx
vtkQuadraticTriangle.cxx
vtkQuadraticWedge.cxx
vtkRectilinearGrid.cxx
vtkRectilinearGridSource.cxx
vtkRectilinearGridToPolyDataFilter.cxx
vtkScalarTree.cxx
vtkSimpleImageToImageFilter.cxx
vtkSimpleScalarTree.cxx
vtkSphere.cxx
vtkSource.cxx
vtkSpline.cxx
vtkStreamingDemandDrivenPipeline.cxx
vtkStructuredGrid.cxx
vtkStructuredGridSource.cxx
vtkStructuredGridToPolyDataFilter.cxx
vtkStructuredGridToStructuredGridFilter.cxx
vtkStructuredPoints.cxx
vtkStructuredPointsCollection.cxx
vtkStructuredPointsSource.cxx
vtkStructuredPointsToPolyDataFilter.cxx
vtkStructuredPointsToStructuredPointsFilter.cxx
vtkStructuredPointsToUnstructuredGridFilter.cxx
vtkSuperquadric.cxx
vtkTetra.cxx
vtkThreadedImageAlgorithm.cxx
vtkTriangle.cxx
vtkTriangleStrip.cxx
vtkTrivialProducer.cxx
vtkUniformGrid.cxx
vtkUnstructuredGrid.cxx
vtkUnstructuredGridAlgorithm.cxx
vtkUnstructuredGridSource.cxx
vtkUnstructuredGridToPolyDataFilter.cxx
vtkUnstructuredGridToUnstructuredGridFilter.cxx
vtkVertex.cxx
vtkViewDependentErrorMetric.cxx
vtkVoxel.cxx
vtkWedge.cxx
vtkViewport.cxx
vtkGenericInterpolatedVelocityField.cxx
)

SET_SOURCE_FILES_PROPERTIES(
vtkAbstractMapper
vtkCell
vtkCell3D
vtkCompositeDataIterator
vtkCompositeDataSet
vtkDataObjectSource
vtkDataSet
vtkDataSetSource
vtkDataSetToDataSetFilter
vtkDataSetToImageFilter
vtkDataSetToPolyDataFilter
vtkDataSetToStructuredGridFilter
vtkDataSetToStructuredPointsFilter
vtkDataSetToUnstructuredGridFilter
vtkExecutive
vtkExplicitCell
vtkFilteringInformationKeyManager
vtkGenericAdaptorCell
vtkGenericAttribute
vtkGenericCellIterator
vtkGenericCellTessellator
vtkGenericDataSet
vtkGenericDataSetSource
vtkGenericDataSetToUnstructuredGridFilter
vtkGenericDataSetToPolyDataFilter
vtkGenericPointIterator
vtkGenericSubdivisionErrorMetric
vtkImageAlgorithm
vtkImageInPlaceFilter
vtkImageIterator
vtkImageMultipleInputFilter
vtkImageMultipleInputOutputFilter
vtkImageProgressIterator
vtkImageSource
vtkImageToImageFilter
vtkImageTwoInputFilter
vtkInformationDataObjectKey
vtkInformationDoubleKey
vtkInformationDoubleVectorKey
vtkInformationExecutivePortKey
vtkInformationExecutivePortVectorKey
vtkInformationInformationKey
vtkInformationInformationVectorKey
vtkInformationIntegerKey
vtkInformationIntegerVectorKey
vtkInformationKey
vtkInformationKeyVectorKey
vtkInformationObjectBaseKey
vtkInformationStringKey
vtkInformationUnsignedLongKey
vtkLocator
vtkMapper2D
vtkNonLinearCell
vtkPiecewiseFunctionSource
vtkPiecewiseFunctionToPiecewiseFunctionFilter
vtkPointSet
vtkPointSetSource
vtkPointSetToPointSetFilter
vtkPolyDataSource
vtkPolyDataToPolyDataFilter
vtkProcessObject
vtkRectilinearGridSource
vtkRectilinearGridToPolyDataFilter
vtkScalarTree
vtkSimpleImageToImageFilter
vtkSource
vtkSpline
vtkStructuredGridSource
vtkStructuredGridToPolyDataFilter
vtkStructuredGridToStructuredGridFilter
vtkStructuredPointsSource
vtkStructuredPointsToPolyDataFilter
vtkStructuredPointsToStructuredPointsFilter
vtkStructuredPointsToUnstructuredGridFilter
vtkThreadedImageAlgorithm
vtkUnstructuredGridSource
vtkUnstructuredGridToPolyDataFilter
vtkUnstructuredGridToUnstructuredGridFilter
vtkViewport
  ABSTRACT
)

SET_SOURCE_FILES_PROPERTIES(
  vtkFilteringInformationKeyManager.cxx
  vtkImageIterator.cxx
  vtkImageProgressIterator.cxx
  WRAP_EXCLUDE
)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)

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
  #VTK_WRAP_TCL3(vtk${KIT}TCL KitTCL_SRCS 
  #              "${Kit_SRCS}"
  #              "${Kit_TCL_EXTRA_CMDS}")
  VTK_WRAP_TCL2(vtk${KIT}TCL SOURCES KitTCL_SRCS 
                ${Kit_SRCS}
                COMMANDS ${Kit_TCL_EXTRA_CMDS})
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS} ${Kit_TCL_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} ${KIT_TCL_LIBS})
  INSTALL_TARGETS(/lib/vtk vtk${KIT}TCL)
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
    INSTALL_TARGETS(/lib/vtk vtk${KIT}PythonD)
  ELSE (APPLE)
    ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    TARGET_LINK_LIBRARIES (vtk${KIT}Python vtk${KIT} ${KIT_PYTHON_LIBS})
  ENDIF(APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python ${VTK_PYTHON_LIBRARIES})
  ENDIF(WIN32)
  INSTALL_TARGETS(/lib/vtk vtk${KIT}Python)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA3(vtk${KIT}Java KitJava_SRCS "${Kit_SRCS}")
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(vtk${KIT}Java vtk${KIT} ${KIT_JAVA_LIBS})
  INSTALL_TARGETS(/lib/vtk vtk${KIT}Java)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES(vtk${KIT} ${KIT_LIBS})

INSTALL_TARGETS(/lib/vtk vtk${KIT})
INSTALL_FILES(/include/vtk .h ${Kit_SRCS})

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

INSTALL_FILES(/include/vtk .h
  vtkAMRBox
  vtkCellType
)

INSTALL_FILES(/include/vtk .txx
  vtkImageIterator
  vtkImageProgressIterator
)
