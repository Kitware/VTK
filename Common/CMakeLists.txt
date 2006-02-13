SET(KIT Common)
SET(UKIT COMMON)
SET(KIT_TCL_LIBS ${VTK_TCL_LIBRARIES})
SET(KIT_PYTHON_LIBS)
SET(KIT_JAVA_LIBS)
SET(KIT_LIBS vtksys)

IF(WIN32)
  IF (NOT VTK_USE_X)
    SET(VTK_OWP_ENCODED_C vtkWin32OutputWindowProcessEncoded.c)
    SET_SOURCE_FILES_PROPERTIES(${VTK_OWP_ENCODED_C} PROPERTIES WRAP_EXCLUDE 1)
  ENDIF (NOT VTK_USE_X)
ENDIF(WIN32)

IF (APPLE)
  SET(KIT_LIBS ${KIT_LIBS} "-framework AppKit")
ENDIF (APPLE)
IF (UNIX)
  SET(KIT_LIBS ${KIT_LIBS} ${CMAKE_THREAD_LIBS} ${CMAKE_DL_LIBS} -lm)
ENDIF (UNIX)

SET( Kit_SRCS
vtkAbstractArray.cxx
vtkAbstractTransform.cxx
vtkAmoebaMinimizer.cxx
vtkAnimationCue.cxx
vtkAnimationScene.cxx
vtkArrayIterator.cxx
vtkAssemblyNode.cxx
vtkAssemblyPath.cxx
vtkAssemblyPaths.cxx
vtkBitArray.cxx
vtkBitArrayIterator.cxx
vtkBox.cxx
vtkByteSwap.cxx
vtkCallbackCommand.cxx
vtkCharArray.cxx
vtkCollection.cxx
vtkCollectionIterator.cxx
vtkCommand.cxx
vtkContourValues.cxx
vtkCriticalSection.cxx
vtkCylindricalTransform.cxx
vtkDataArray.cxx
vtkDataArrayCollection.cxx
vtkDataArrayCollectionIterator.cxx
vtkDataArraySelection.cxx
vtkDebugLeaks.cxx
vtkDebugLeaksManager.cxx
vtkDirectory.cxx
vtkDoubleArray.cxx
vtkDynamicLoader.cxx
vtkEdgeTable.cxx
vtkErrorCode.cxx
vtkEventForwarderCommand.cxx
vtkExtentSplitter.cxx
vtkExtentTranslator.cxx
vtkFastNumericConversion.cxx
vtkFileOutputWindow.cxx
vtkFloatArray.cxx
vtkFunctionParser.cxx
vtkFunctionSet.cxx
vtkGarbageCollector.cxx
vtkGarbageCollectorManager.cxx
vtkGeneralTransform.cxx
vtkHeap.cxx
vtkHomogeneousTransform.cxx
vtkIOStream.cxx
vtkIdList.cxx
vtkIdListCollection.cxx
vtkIdTypeArray.cxx
vtkIdentityTransform.cxx
vtkImplicitFunction.cxx
vtkImplicitFunctionCollection.cxx
vtkIndent.cxx
vtkInitialValueProblemSolver.cxx
vtkInstantiator.cxx
vtkIntArray.cxx
vtkLargeInteger.cxx
vtkLinearTransform.cxx
vtkLogLookupTable.cxx
vtkLongArray.cxx
vtkLookupTable.cxx
vtkMath.cxx
vtkMatrix4x4.cxx
vtkMatrixToHomogeneousTransform.cxx
vtkMatrixToLinearTransform.cxx
vtkMultiPartExtentTranslator.cxx
vtkMultiThreader.cxx
vtkMutexLock.cxx
vtkOStrStreamWrapper.cxx
vtkOStreamWrapper.cxx
vtkObject.cxx
vtkObjectBase.cxx
vtkObjectFactory.cxx
vtkObjectFactoryCollection.cxx
vtkOldStyleCallbackCommand.cxx
vtkOutputWindow.cxx
vtkOverrideInformation.cxx
vtkOverrideInformationCollection.cxx
vtkParametricBoy.cxx
vtkParametricConicSpiral.cxx
vtkParametricCrossCap.cxx
vtkParametricDini.cxx
vtkParametricEllipsoid.cxx
vtkParametricEnneper.cxx
vtkParametricFigure8Klein.cxx
vtkParametricFunction.cxx
vtkParametricKlein.cxx
vtkParametricMobius.cxx
vtkParametricRandomHills.cxx
vtkParametricRoman.cxx
vtkParametricSuperEllipsoid.cxx
vtkParametricSuperToroid.cxx
vtkParametricTorus.cxx
vtkPerspectiveTransform.cxx
vtkPlane.cxx
vtkPlaneCollection.cxx
vtkPlanes.cxx
vtkPoints.cxx
vtkPriorityQueue.cxx
vtkProp.cxx
vtkPropCollection.cxx
vtkProperty2D.cxx
vtkQuadric.cxx
vtkReferenceCount.cxx
vtkRungeKutta2.cxx
vtkRungeKutta4.cxx
vtkRungeKutta45.cxx
vtkScalarsToColors.cxx
vtkShortArray.cxx
vtkSignedCharArray.cxx
vtkSmartPointerBase.cxx
vtkSphericalTransform.cxx
vtkStdString.cxx
vtkStringArray.cxx
vtkStructuredData.cxx
vtkStructuredVisibilityConstraint.cxx
vtkTableExtentTranslator.cxx
vtkTensor.cxx
vtkThreadMessager.cxx
vtkTimeStamp.cxx
vtkTimerLog.cxx
vtkTransform.cxx
vtkTransformCollection.cxx
vtkUnsignedCharArray.cxx
vtkUnsignedIntArray.cxx
vtkUnsignedLongArray.cxx
vtkUnsignedShortArray.cxx
vtkVersion.cxx
vtkVoidArray.cxx
vtkWarpTransform.cxx
vtkWindow.cxx
vtkWindowLevelLookupTable.cxx
vtkXMLFileOutputWindow.cxx
)

IF(VTK_TYPE_USE_LONG_LONG)
  SET(Kit_SRCS ${Kit_SRCS}
    vtkLongLongArray.cxx
    vtkUnsignedLongLongArray.cxx
    )
ENDIF(VTK_TYPE_USE_LONG_LONG)
IF(VTK_TYPE_USE___INT64)
  SET(Kit_SRCS ${Kit_SRCS} vtk__Int64Array.cxx)
  IF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    SET(Kit_SRCS ${Kit_SRCS} vtkUnsigned__Int64Array.cxx)
  ENDIF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
ENDIF(VTK_TYPE_USE___INT64)

# Configure data arrays for platform-independent fixed-size types.
# Match the type selection here to that in vtkType.h.
IF(VTK_TYPE_CHAR_IS_SIGNED)
  SET(VTK_TYPE_NATIVE_Int8 Char)
ELSE(VTK_TYPE_CHAR_IS_SIGNED)
  SET(VTK_TYPE_NATIVE_Int8 SignedChar)
ENDIF(VTK_TYPE_CHAR_IS_SIGNED)
SET(VTK_TYPE_NATIVE_Int16 UnsignedShort)
SET(VTK_TYPE_NATIVE_Int32 UnsignedInt)
SET(VTK_TYPE_NATIVE_UInt8 UnsignedChar)
SET(VTK_TYPE_NATIVE_UInt16 UnsignedShort)
SET(VTK_TYPE_NATIVE_UInt32 UnsignedInt)
IF(VTK_SIZEOF_LONG MATCHES "^8$")
  SET(VTK_TYPE_NATIVE_Int64 Long)
  SET(VTK_TYPE_NATIVE_UInt64 UnsignedLong)
ELSE(VTK_SIZEOF_LONG MATCHES "^8$")
  IF(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
    SET(VTK_TYPE_NATIVE_Int64 LongLong)
    SET(VTK_TYPE_NATIVE_UInt64 UnsignedLongLong)
  ELSE(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
    IF(VTK_SIZEOF___INT64 MATCHES "^8$")
      SET(VTK_TYPE_NATIVE_Int64 __Int64)
      IF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
        SET(VTK_TYPE_NATIVE_UInt64 Unsigned__Int64)
      ENDIF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    ENDIF(VTK_SIZEOF___INT64 MATCHES "^8$")
  ENDIF(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
ENDIF(VTK_SIZEOF_LONG MATCHES "^8$")
SET(VTK_TYPE_NATIVE_Float32 Float)
SET(VTK_TYPE_NATIVE_Float64 Double)
FOREACH(t Int8 Int16 Int32 Int64 UInt8 UInt16 UInt32 UInt64 Float32 Float64)
  SET(VTK_TYPE_NAME ${t})
  SET(VTK_TYPE_NATIVE ${VTK_TYPE_NATIVE_${t}})
  IF(VTK_TYPE_NATIVE)
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkTypedArray.h.in
                   ${CMAKE_CURRENT_BINARY_DIR}/vtkType${t}Array.h
                   @ONLY IMMEDIATE)
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkTypedArray.cxx.in
                   ${CMAKE_CURRENT_BINARY_DIR}/vtkType${t}Array.cxx
                   @ONLY IMMEDIATE)
    SET(Kit_SRCS ${Kit_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/vtkType${t}Array.cxx)
  ENDIF(VTK_TYPE_NATIVE)
ENDFOREACH(t)

SET_SOURCE_FILES_PROPERTIES(
# vtkObjectBase is marked ABSTRACT so that instantiators are happy.
vtkAbstractArray
vtkArrayIterator
vtkArrayIteratorTemplate
vtkAbstractTransform
vtkCallbackCommand
vtkCommand
vtkDataArray
vtkEventForwarderCommand
vtkFunctionSet
vtkHomogeneousTransform
vtkImplicitFunction
vtkInitialValueProblemSolver
vtkLinearTransform
vtkObjectBase
vtkObjectFactory
vtkOldStyleCallbackCommand
vtkOverrideInformation
vtkOverrideInformationCollection
vtkParametricFunction
vtkProp
vtkScalarsToColors
vtkStructuredData
vtkWarpTransform
vtkWindow
ABSTRACT
)

SET_SOURCE_FILES_PROPERTIES(
  vtkArrayIteratorTemplate.txx
  vtkCallbackCommand.cxx
  vtkCommand.cxx
  vtkDebugLeaksManager.cxx
  vtkErrorCode.cxx
  vtkEventForwarderCommand.cxx
  vtkGarbageCollectorManager.cxx
  vtkIOStream.cxx
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
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

IF (WIN32)
  IF (NOT VTK_USE_X)
    SET(Kit_SRCS ${Kit_SRCS} vtkWin32OutputWindow.cxx
      vtkWin32ProcessOutputWindow.cxx)
    SET(Kit_EXTRA_SRCS ${Kit_EXTRA_SRCS} vtkWin32OutputWindowProcessEncoded.c)
    SET_SOURCE_FILES_PROPERTIES(vtkWin32OutputWindowProcessEncoded.c
      PROPERTIES WRAP_EXCLUDE 1)
  ENDIF (NOT VTK_USE_X)
ENDIF (WIN32)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h
    vtkArrayIteratorTemplate
    vtkDataArrayTemplate
    vtkIOStream
    vtkIOStreamFwd
    vtkSetGet
    vtkSmartPointer
    vtkSystemIncludes
    vtkType
    vtkTypeTraits
    vtkWin32Header
    vtkWindows
    )

  IF(VTK_WRAP_TCL)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h
      vtkTcl
      vtkTclUtil
      vtkTk
      )
  ENDIF(VTK_WRAP_TCL)
  IF(VTK_WRAP_PYTHON)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h
      vtkPythonUtil
      )
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} FILES
      vtkPython.h
      )
    IF (TK_LIBRARY)
      IF(NOT VTK_USE_COCOA)
        INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h
          vtkTcl
          vtkTk
          )
      ENDIF(NOT VTK_USE_COCOA)
    ENDIF (TK_LIBRARY)
  ENDIF(VTK_WRAP_PYTHON)
  IF(VTK_WRAP_JAVA)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h
      vtkJavaUtil
      )
  ENDIF(VTK_WRAP_JAVA)

  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .txx
    vtkArrayIteratorTemplate
    vtkArrayIteratorTemplateImplicit
    vtkDataArrayTemplate
    vtkDataArrayTemplateImplicit
    )

  INSTALL_FILES(${VTK_INSTALL_PACKAGE_DIR}/testing FILES
    ${VTK_SOURCE_DIR}/${KIT}/Testing/HeaderTesting.py
    ${VTK_SOURCE_DIR}/${KIT}/Testing/WindowsMangleList.py
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/FindString.tcl
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/PrintSelfCheck.tcl
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/rtImageTest.tcl
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/TestSetGet.tcl
    )
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} FILES
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Cxx/vtkTestUtilities.h)
  IF(VTK_USE_PARALLEL)
    INSTALL_FILES(${VTK_INSTALL_PACKAGE_DIR}/testing FILES
      ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/prtImageTest.tcl)
  ENDIF(VTK_USE_PARALLEL)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
