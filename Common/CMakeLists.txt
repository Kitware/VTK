SET(KIT Common)
SET(UKIT COMMON)
SET(KIT_TCL_LIBS ${TCL_LIBRARY})
SET(KIT_PYTHON_LIBS)
SET(KIT_JAVA_LIBS)
SET(KIT_LIBS)

IF (APPLE)
  SET(KIT_LIBS ${KIT_LIBS} "-framework AppKit")
ENDIF (APPLE)
IF (UNIX)
  SET(KIT_LIBS ${KIT_LIBS} ${CMAKE_THREAD_LIBS} ${CMAKE_DL_LIBS} -lm)
ENDIF (UNIX)

SET( Kit_SRCS
vtkAbstractMapper.cxx
vtkAbstractTransform.cxx
vtkActor2D.cxx
vtkActor2DCollection.cxx
vtkAmoebaMinimizer.cxx
vtkAssemblyNode.cxx
vtkAssemblyPath.cxx
vtkAssemblyPaths.cxx
vtkBitArray.cxx
vtkBox.cxx
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
vtkCompositeDataIterator.cxx
vtkCompositeDataSet.cxx
vtkCompositeDataVisitor.cxx
vtkCompositeDataCommand.cxx
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
vtkExtentSplitter.cxx
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
vtkHierarchicalBoxDataSet.cxx
vtkHierarchicalBoxVisitor.cxx
vtkHierarchicalDataSet.cxx
vtkHierarchicalDataIterator.cxx
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
vtkMultiBlockDataIterator.cxx
vtkMultiBlockDataSet.cxx
vtkMultiBlockDataVisitor.cxx
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
vtkStdString.cxx
vtkStructuredData.cxx
vtkStructuredGrid.cxx
vtkStructuredPoints.cxx
vtkStructuredVisibilityConstraint.cxx
vtkTableExtentTranslator.cxx
vtkTensor.cxx
vtkTetra.cxx
vtkTimeStamp.cxx
vtkTimerLog.cxx
vtkTransform.cxx
vtkTransformCollection.cxx
vtkTriangle.cxx
vtkTriangleStrip.cxx
vtkUniformGrid.cxx
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

IF (WIN32)
  IF (NOT VTK_USE_X)
    SET(Kit_SRCS ${Kit_SRCS} vtkWin32OutputWindow.cxx)
  ENDIF (NOT VTK_USE_X)
ENDIF (WIN32)

SET_SOURCE_FILES_PROPERTIES(
# vtkObjectBase is marked ABSTRACT so that instantiators are happy.
vtkAbstractMapper
vtkAbstractTransform
vtkCallbackCommand
vtkCell
vtkCell3D
vtkCommand
vtkCompositeDataIterator
vtkCompositeDataSet
vtkCompositeDataVisitor
vtkCompositeDataCommand
vtkDataArray
vtkDataSet
vtkExplicitCell
vtkFunctionSet
vtkHierarchicalDataSet
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

SET_SOURCE_FILES_PROPERTIES(
  vtkCallbackCommand.cxx
  vtkCommand.cxx
  vtkDebugLeaksManager.cxx
  vtkErrorCode.cxx
  vtkIdType.cxx
  vtkImageIterator.cxx
  vtkImageProgressIterator.cxx
  vtkIndent.cxx
  vtkLargeInteger.cxx
  vtkOStrStreamWrapper.cxx
  vtkOStreamWrapper.cxx
  vtkOldStyleCallbackCommand.cxx
  vtkSmartPointerBase.cxx
  vtkStdString.cxx
  vtkTimeStamp.cxx
  WRAP_EXCLUDE
)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS vtkTclUtil.cxx)
SET(Kit_PYTHON_EXTRA_SRCS vtkPythonUtil.cxx)
SET(Kit_JAVA_EXTRA_SRCS vtkJavaUtil.cxx)

# -----------------------------------------------------------------
# Start of common section

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtk${KIT}Instantiator KitInstantiator_SRCS
                       ${Kit_SRCS}
                       EXPORT_MACRO VTK_${UKIT}_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${Kit_EXTRA_SRCS} ${KitInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtk${KIT} PROPERTIES LINK_FLAGS "-init _oft_init${KIT}")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

# Allow the user to customize their build with some local options
#
SET(LOCALUSERMACRODEFINED 0)
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
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
  VTK_WRAP_PYTHON2(vtk${KIT}Python KitPython_SRCS ${Kit_SRCS})
  IF (APPLE)
    ADD_LIBRARY(vtk${KIT}PythonD ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    ADD_LIBRARY(vtk${KIT}Python MODULE vtk${KIT}PythonInit.cxx)
    TARGET_LINK_LIBRARIES(vtk${KIT}PythonD vtk${KIT} ${KIT_PYTHON_LIBS} ${PYTHON_LIBRARY})
    TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT}PythonD)
    INSTALL_TARGETS(/lib/vtk vtk${KIT}PythonD)
  ELSE (APPLE)
    ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    TARGET_LINK_LIBRARIES (vtk${KIT}Python vtk${KIT} ${KIT_PYTHON_LIBS})
  ENDIF(APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  INSTALL_TARGETS(/lib/vtk vtk${KIT}Python)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtk${KIT}Java KitJava_SRCS ${Kit_SRCS})
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
  vtkCellType
  vtkDataArrayTemplate
  vtkIOStream
  vtkIOStreamFwd
  vtkSetGet
  vtkSmartPointer
  vtkSystemIncludes
  vtkWin32Header
)

IF(VTK_WRAP_TCL)
  INSTALL_FILES(/include/vtk .h
    vtkTcl
    vtkTclUtil
    vtkTk
  )
ENDIF(VTK_WRAP_TCL)
IF(VTK_WRAP_PYTHON)
  INSTALL_FILES(/include/vtk .h
    vtkPythonUtil
  )
  IF (TK_LIBRARY)
	IF(NOT VTK_USE_COCOA)
      INSTALL_FILES(/include/vtk .h
        vtkTcl
        vtkTk
       )
    ENDIF(NOT VTK_USE_COCOA)
  ENDIF (TK_LIBRARY)
ENDIF(VTK_WRAP_PYTHON)
IF(VTK_WRAP_JAVA)
  INSTALL_FILES(/include/vtk .h
    vtkJavaUtil
  )
ENDIF(VTK_WRAP_JAVA)

INSTALL_FILES(/include/vtk .txx
  vtkDataArrayTemplate
  vtkImageIterator
  vtkImageProgressIterator
)

INSTALL_FILES(/lib/vtk/testing FILES
              ${VTK_SOURCE_DIR}/${KIT}/Testing/HeaderTesting.py
              ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/FindString.tcl
              ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/PrintSelfCheck.tcl
              ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/rtImageTest.tcl)
INSTALL_FILES(/include/vtk FILES
              ${VTK_SOURCE_DIR}/${KIT}/Testing/Cxx/vtkRegressionTestImage.h)
IF(VTK_USE_PARALLEL)
  INSTALL_FILES(/lib/vtk/testing FILES
                ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/prtImageTest.tcl)
ENDIF(VTK_USE_PARALLEL)
