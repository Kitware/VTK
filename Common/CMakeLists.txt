SOURCE_FILES( Common_SRCS
vtkAbstractMapper.cxx
vtkAbstractTransform.cxx
vtkActor2D.cxx
vtkActor2DCollection.cxx
vtkAssemblyNode.cxx
vtkAssemblyPath.cxx
vtkAssemblyPaths.cxx
vtkBitArray.cxx
vtkByteSwap.cxx
vtkCallbackCommand.cxx
vtkCell.cxx
vtkCell3D.cxx
vtkCellArray.cxx
vtkCellData.cxx
vtkCellLinks.cxx
vtkCellTypes.cxx
vtkCharArray.cxx
vtkCollection.cxx
vtkCommand.cxx
vtkContainer.cxx
vtkContourValues.cxx
vtkConvexPointSet.cxx
vtkCoordinate.cxx
vtkCriticalSection.cxx
vtkCylindricalTransform.cxx
vtkDataArray.cxx
vtkDataObject.cxx
vtkDataObjectCollection.cxx
vtkDataSet.cxx
vtkDataSetAttributes.cxx
vtkDataSetCollection.cxx
vtkDebugLeaks.cxx
vtkDirectory.cxx
vtkDoubleArray.cxx
vtkDynamicLoader.cxx
vtkEdgeTable.cxx
vtkEmptyCell.cxx
vtkErrorCode.cxx
vtkExplicitCell.cxx
vtkExtentTranslator.cxx
vtkFieldData.cxx
vtkFileOutputWindow.cxx
vtkFloatArray.cxx
vtkFunctionParser.cxx
vtkFunctionSet.cxx
vtkGeneralTransform.cxx
vtkGenericCell.cxx
vtkHeap.cxx
vtkHexahedron.cxx
vtkHomogeneousTransform.cxx
vtkIdList.cxx
vtkIdTypeArray.cxx
vtkIdentityTransform.cxx
vtkImageData.cxx
vtkImageIterator.cxx
vtkImageProgressIterator.cxx
vtkImplicitFunction.cxx
vtkImplicitFunctionCollection.cxx
vtkIndent.cxx
vtkInitialValueProblemSolver.cxx
vtkInstantiator.cxx
vtkIntArray.cxx
vtkInterpolatedVelocityField.cxx
vtkLargeInteger.cxx
vtkLine.cxx
vtkLinearTransform.cxx
vtkLocator.cxx
vtkLogLookupTable.cxx
vtkLongArray.cxx
vtkLookupTable.cxx
vtkMapper2D.cxx
vtkMath.cxx
vtkMatrix4x4.cxx
vtkMatrixToHomogeneousTransform.cxx
vtkMatrixToLinearTransform.cxx
vtkMultiThreader.cxx
vtkMutexLock.cxx
vtkNonLinearCell.cxx
vtkObject.cxx
vtkObjectFactory.cxx
vtkObjectFactoryCollection.cxx
vtkOldStyleCallbackCommand.cxx
vtkOrderedTriangulator.cxx
vtkOutputWindow.cxx
vtkOverrideInformation.cxx
vtkOverrideInformationCollection.cxx
vtkPerspectiveTransform.cxx
vtkPixel.cxx
vtkPlane.cxx
vtkPlaneCollection.cxx
vtkPlanes.cxx
vtkPointData.cxx
vtkPointLocator.cxx
vtkPointLocator2D.cxx
vtkPointSet.cxx
vtkPoints.cxx
vtkPolyData.cxx
vtkPolyLine.cxx
vtkPolyVertex.cxx
vtkPolygon.cxx
vtkPriorityQueue.cxx
vtkProcessObject.cxx
vtkProp.cxx
vtkPropAssembly.cxx
vtkPropCollection.cxx
vtkProperty2D.cxx
vtkPyramid.cxx
vtkQuad.cxx
vtkQuadraticEdge.cxx
vtkQuadraticTriangle.cxx
vtkQuadraticQuad.cxx
vtkQuadraticTetra.cxx
vtkQuadraticHexahedron.cxx
vtkQuadric.cxx
vtkRectilinearGrid.cxx
vtkReferenceCount.cxx
vtkRungeKutta2.cxx
vtkRungeKutta4.cxx
vtkRungeKutta45.cxx
vtkScalarsToColors.cxx
vtkShortArray.cxx
vtkSource.cxx
vtkSphericalTransform.cxx
vtkString.cxx
vtkStructuredData.cxx
vtkStructuredGrid.cxx
vtkStructuredPoints.cxx
vtkTensor.cxx
vtkTetra.cxx
vtkTimeStamp.cxx
vtkTimerLog.cxx
vtkTransform.cxx
vtkTransformCollection.cxx
vtkTriangle.cxx
vtkTriangleStrip.cxx
vtkUnsignedCharArray.cxx
vtkUnsignedIntArray.cxx
vtkUnsignedLongArray.cxx
vtkUnsignedShortArray.cxx
vtkUnstructuredGrid.cxx
vtkVersion.cxx
vtkVertex.cxx
vtkViewport.cxx
vtkVoidArray.cxx
vtkVoxel.cxx
vtkWarpTransform.cxx
vtkWedge.cxx
vtkWindow.cxx
vtkWindowLevelLookupTable.cxx
vtkXMLFileOutputWindow.cxx
)

ABSTRACT_FILES(
vtkAbstractMapper
vtkAbstractTransform
vtkCallbackCommand
vtkCell
vtkCell3D
vtkCommand
vtkContainer
vtkDataArray
vtkDataSet
vtkExplicitCell
vtkFunctionSet
vtkHomogeneousTransform
vtkImplicitFunction
vtkInitialValueProblemSolver
vtkLinearTransform
vtkLocator
vtkNonLinearCell
vtkObjectFactory
vtkOldStyleCallbackCommand
vtkOverrideInformation
vtkOverrideInformationCollection
vtkPointSet
vtkScalarsToColors
vtkSource
vtkViewport
vtkWarpTransform
vtkWindow
)

IF (WIN32)
  SOURCE_FILES(Common_SRCS vtkWin32OutputWindow.cxx)
ENDIF (WIN32)

WRAP_EXCLUDE_FILES(
  vtkCallbackCommand
  vtkCommand
  vtkContainer
  vtkErrorCode
  vtkImageIterator
  vtkImageProgressIterator
  vtkLargeInteger
  vtkOldStyleCallbackCommand
  vtkString
)

IF(VTK_MAKE_INSTANTIATOR)
  # Setup vtkInstantiator registration for this library's classes.
  VTK_MAKE_INSTANTIATOR(vtkCommonInstantiator CommonInstantiator_SRCS
                        Common_SRCS
                        EXPORT_MACRO VTK_COMMON_EXPORT
                        HEADER_LOCATION ${VTK_BINARY_DIR})
  ADD_LIBRARY(vtkCommon Common_SRCS CommonInstantiator_SRCS)
  INSTALL_FILES(/include/vtk .h ${VTK_BINARY_DIR}/vtkCommonInstantiator)
ELSE(VTK_MAKE_INSTANTIATOR)
  ADD_LIBRARY(vtkCommon Common_SRCS)
ENDIF(VTK_MAKE_INSTANTIATOR)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Common/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Common/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkCommonTCL CommonTCL_SRCS Common_SRCS)
  IF (APPLE)
    ADD_LIBRARY(vtkCommonTCL SHARED CommonTCL_SRCS vtkTclUtil)
  ELSE (APPLE)
    ADD_LIBRARY(vtkCommonTCL CommonTCL_SRCS vtkTclUtil)
  ENDIF (APPLE)
  TARGET_LINK_LIBRARIES (vtkCommonTCL ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkCommonTCL)
  INSTALL_FILES(/include/vtk .h
    vtkTclUtil
  )
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkCommonPython CommonPython_SRCS Common_SRCS)
  IF (APPLE)
    ADD_LIBRARY(vtkCommonPython MODULE CommonPython_SRCS vtkPythonUtil)
  ELSE (APPLE)
    ADD_LIBRARY(vtkCommonPython SHARED CommonPython_SRCS vtkPythonUtil)
  ENDIF(APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkCommonPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtkCommonPython ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  INSTALL_TARGETS(/lib/vtk vtkCommonPython)
  INSTALL_FILES(/include/vtk .h
    vtkPythonUtil
  )
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkCommonJava CommonJava_SRCS Common_SRCS)
  ADD_LIBRARY(vtkCommonJava SHARED CommonJava_SRCS vtkJavaUtil)
  INSTALL_TARGETS(/lib/vtk vtkCommonJava)
  INSTALL_FILES(/include/vtk .h
    vtkJavaUtil
  )
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (vtkCommon )

INSTALL_TARGETS(/lib/vtk vtkCommon)
INSTALL_FILES(/include/vtk .h Common_SRCS
  vtkCellType
  vtkSetGet
  vtkSystemIncludes
  vtkWin32Header
  )

