SET( Common_SRCS
vtkAbstractMapper.cxx
vtkAbstractTransform.cxx
vtkActor2D.cxx
vtkActor2DCollection.cxx
vtkAmoebaMinimizer.cxx
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
vtkCollectionIterator.cxx
vtkCommand.cxx
vtkContourValues.cxx
vtkConvexPointSet.cxx
vtkCoordinate.cxx
vtkCriticalSection.cxx
vtkCylindricalTransform.cxx
vtkDataArray.cxx
vtkDataArrayCollection.cxx
vtkDataArrayCollectionIterator.cxx
vtkDataArraySelection.cxx
vtkDataObject.cxx
vtkDataObjectCollection.cxx
vtkDataSet.cxx
vtkDataSetAttributes.cxx
vtkDataSetCollection.cxx
vtkDebugLeaks.cxx
vtkDebugLeaksManager.cxx
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
vtkIdListCollection.cxx
vtkIdType.cxx
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
vtkMultiPartExtentTranslator.cxx
vtkMultiThreader.cxx
vtkMutexLock.cxx
vtkNonLinearCell.cxx
vtkOStrStreamWrapper.cxx
vtkOStreamWrapper.cxx
vtkObject.cxx
vtkObjectBase.cxx
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
vtkQuadraticHexahedron.cxx
vtkQuadraticQuad.cxx
vtkQuadraticTetra.cxx
vtkQuadraticTriangle.cxx
vtkQuadric.cxx
vtkRectilinearGrid.cxx
vtkReferenceCount.cxx
vtkRungeKutta2.cxx
vtkRungeKutta4.cxx
vtkRungeKutta45.cxx
vtkScalarsToColors.cxx
vtkShortArray.cxx
vtkSmartPointerBase.cxx
vtkSource.cxx
vtkSphericalTransform.cxx
vtkStructuredData.cxx
vtkStructuredGrid.cxx
vtkStructuredPoints.cxx
vtkTableExtentTranslator.cxx
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


# vtkObjectBase is marked ABSTRACT so that instantiators are happy.
SET_SOURCE_FILES_PROPERTIES(
vtkAbstractMapper
vtkAbstractTransform
vtkCallbackCommand
vtkCell
vtkCell3D
vtkCommand
vtkDataArray
vtkDataSet
vtkExplicitCell
vtkFunctionSet
vtkHomogeneousTransform
vtkImplicitFunction
vtkInitialValueProblemSolver
vtkLinearTransform
vtkLocator
vtkMapper2D
vtkNonLinearCell
vtkObjectBase
vtkObjectFactory
vtkOldStyleCallbackCommand
vtkOverrideInformation
vtkOverrideInformationCollection
vtkPointSet
vtkProcessObject
vtkProp
vtkScalarsToColors
vtkSource
vtkStructuredData
vtkViewport
vtkWarpTransform
vtkWindow
ABSTRACT
)

IF (WIN32)
  SET(Common_SRCS ${Common_SRCS} vtkWin32OutputWindow.cxx)
ENDIF (WIN32)

SET_SOURCE_FILES_PROPERTIES(
  vtkCallbackCommand
  vtkCommand
  vtkDebugLeaksManager
  vtkErrorCode
  vtkIdType.cxx
  vtkImageIterator
  vtkImageProgressIterator
  vtkLargeInteger
  vtkOStrStreamWrapper.cxx
  vtkOStreamWrapper.cxx
  vtkOldStyleCallbackCommand
  vtkSmartPointerBase.cxx
  WRAP_EXCLUDE
)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkCommonInstantiator CommonInstantiator_SRCS
                       ${Common_SRCS}
                       EXPORT_MACRO VTK_COMMON_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtkCommon ${Common_SRCS} ${CommonInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtkCommon PROPERTIES LINK_FLAGS "-init _oft_initCom")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)
IF (APPLE)
  TARGET_LINK_LIBRARIES(vtkCommon "-framework AppKit")
ENDIF (APPLE)

IF (UNIX)
  TARGET_LINK_LIBRARIES(vtkCommon ${CMAKE_THREAD_LIBS} ${CMAKE_DL_LIBS} -lm)
ENDIF (UNIX)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Common/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Common/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtkCommonTCL CommonTCL_SRCS ${Common_SRCS})
  ADD_LIBRARY(vtkCommonTCL ${CommonTCL_SRCS} vtkTclUtil.cxx)
  TARGET_LINK_LIBRARIES (vtkCommonTCL ${TCL_LIBRARY} vtkCommon)
  INSTALL_TARGETS(/lib/vtk vtkCommonTCL)
  INSTALL_FILES(/include/vtk .h
    vtkTclUtil
  )
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtkCommonPython CommonPython_SRCS ${Common_SRCS})
  ADD_LIBRARY(vtkCommonPython MODULE ${CommonPython_SRCS} vtkPythonUtil.cxx)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkCommonPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtkCommonPython ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtkCommonPython vtkCommon)

  INSTALL_TARGETS(/lib/vtk vtkCommonPython)
  INSTALL_FILES(/include/vtk .h
    vtkPythonUtil
  )
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtkCommonJava CommonJava_SRCS ${Common_SRCS})
  ADD_LIBRARY(vtkCommonJava SHARED ${CommonJava_SRCS} vtkJavaUtil.cxx)
  TARGET_LINK_LIBRARIES(vtkCommonJava vtkCommon)
  INSTALL_TARGETS(/lib/vtk vtkCommonJava)
  INSTALL_FILES(/include/vtk .h
    vtkJavaUtil
  )
ENDIF (VTK_WRAP_JAVA)

INSTALL_TARGETS(/lib/vtk vtkCommon)
INSTALL_FILES(/include/vtk .h ${Common_SRCS}
  vtkCellType
  vtkSetGet
  vtkSystemIncludes
  vtkWin32Header
  vtkSmartPointer
  vtkIOStream
  vtkIOStreamFwd
  )

INSTALL_FILES(/include/vtk .txx
  vtkImageIterator
  vtkImageProgressIterator
)

INSTALL_FILES(/lib/vtk/testing FILES
              ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
              ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
              ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl
              ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl)
INSTALL_FILES(/include/vtk FILES
              ${VTK_SOURCE_DIR}/Common/Testing/Cxx/vtkRegressionTestImage.h)
IF(VTK_USE_PARALLEL)
  INSTALL_FILES(/lib/vtk/testing FILES
                ${VTK_SOURCE_DIR}/Common/Testing/Tcl/prtImageTest.tcl)
ENDIF(VTK_USE_PARALLEL)
