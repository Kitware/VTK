SET(KIT Common)
SET(UKIT COMMON)
SET(KIT_TCL_LIBS ${VTK_TCL_LIBRARIES})
SET(KIT_PYTHON_LIBS)
SET(KIT_JAVA_LIBS)
SET(KIT_INTERFACE_LIBRARIES ${KWSYS_NAMESPACE})

IF(WIN32)
  IF (NOT VTK_USE_X)
    SET(VTK_OWP_ENCODED_C vtkWin32OutputWindowProcessEncoded.c)
    SET_SOURCE_FILES_PROPERTIES(${VTK_OWP_ENCODED_C} PROPERTIES WRAP_EXCLUDE 1)
  ENDIF (NOT VTK_USE_X)
ENDIF(WIN32)

IF (UNIX)
  SET(KIT_LIBS ${KIT_LIBS} ${CMAKE_THREAD_LIBS} ${CMAKE_DL_LIBS})
  SET(KIT_INTERFACE_LIBRARIES ${KWSYS_NAMESPACE} -lm)
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
vtkBoundingBox.cxx
vtkBox.cxx
vtkBoxMuellerRandomSequence.cxx
vtkBreakPoint.cxx
vtkByteSwap.cxx
vtkCallbackCommand.cxx
vtkCharArray.cxx
vtkClientSocket.cxx
vtkCollection.cxx
vtkCollectionIterator.cxx
vtkColor.h
vtkCommand.cxx
vtkCommonInformationKeyManager.cxx
vtkConditionVariable.cxx
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
vtkFloatingPointExceptions.cxx
vtkFunctionParser.cxx
vtkFunctionSet.cxx
vtkGarbageCollector.cxx
vtkGarbageCollectorManager.cxx
vtkGaussianRandomSequence.cxx
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
vtkInformation.cxx
vtkInformationDataObjectKey.cxx
vtkInformationDoubleKey.cxx
vtkInformationDoubleVectorKey.cxx
vtkInformationIdTypeKey.cxx
vtkInformationInformationKey.cxx
vtkInformationInformationVectorKey.cxx
vtkInformationIntegerKey.cxx
vtkInformationIntegerPointerKey.cxx
vtkInformationIntegerVectorKey.cxx
vtkInformationIterator.cxx
vtkInformationKey.cxx
vtkInformationKeyVectorKey.cxx
vtkInformationObjectBaseKey.cxx
vtkInformationObjectBaseVectorKey.cxx
vtkInformationQuadratureSchemeDefinitionVectorKey.cxx
vtkInformationRequestKey.cxx
vtkInformationStringKey.cxx
vtkInformationStringVectorKey.cxx
vtkInformationUnsignedLongKey.cxx
vtkInformationVector.cxx
vtkInitialValueProblemSolver.cxx
vtkInstantiator.cxx
vtkIntArray.cxx
vtkLargeInteger.cxx
vtkLinearTransform.cxx
vtkLogLookupTable.cxx
vtkLongArray.cxx
vtkLookupTable.cxx
vtkLookupTableWithEnabling.cxx
vtkMath.cxx
vtkMathUtilities.h
vtkMatrix3x3.cxx
vtkMatrix4x4.cxx
vtkMatrixToHomogeneousTransform.cxx
vtkMatrixToLinearTransform.cxx
vtkMinimalStandardRandomSequence.cxx
vtkMultiThreader.cxx
vtkMutexLock.cxx
vtkOStrStreamWrapper.cxx
vtkOStreamWrapper.cxx
vtkObject.cxx
vtkObjectBase.cxx
vtkObjectFactory.cxx
vtkObjectFactoryCollection.cxx
vtkOldStyleCallbackCommand.cxx
vtkOnePieceExtentTranslator.cxx
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
vtkPoints2D.cxx
vtkPolynomialSolversUnivariate.cxx
vtkPriorityQueue.cxx
vtkProp.cxx
vtkPropCollection.cxx
vtkProperty2D.cxx
vtkQuadratureSchemeDefinition.cxx
vtkQuadric.cxx
vtkRandomSequence.cxx
vtkRect.h
vtkReferenceCount.cxx
vtkRungeKutta2.cxx
vtkRungeKutta4.cxx
vtkRungeKutta45.cxx
vtkScalarsToColors.cxx
vtkServerSocket.cxx
vtkShortArray.cxx
vtkSignedCharArray.cxx
vtkSmartPointerBase.cxx
vtkSocket.cxx
vtkSocketCollection.cxx
vtkSortDataArray.cxx
vtkSphericalTransform.cxx
vtkStdString.cxx
vtkStringArray.cxx
vtkStructuredData.cxx
vtkStructuredExtent.cxx
vtkStructuredVisibilityConstraint.cxx
vtkTableExtentTranslator.cxx
vtkTensor.cxx
vtkThreadMessager.cxx
vtkTimePointUtility.cxx
vtkTimeStamp.cxx
vtkTimerLog.cxx
vtkTransform2D.cxx
vtkTransform.cxx
vtkTransformCollection.cxx
vtkUnicodeString.cxx
vtkUnicodeStringArray.cxx
vtkUnsignedCharArray.cxx
vtkUnsignedIntArray.cxx
vtkUnsignedLongArray.cxx
vtkUnsignedShortArray.cxx
vtkVariant.cxx
vtkVariantArray.cxx
vtkVector.h
vtkTuple.h
vtkVersion.cxx
vtkVoidArray.cxx
vtkWarpTransform.cxx
vtkWeakPointerBase.cxx
vtkWindow.cxx
vtkWindowLevelLookupTable.cxx
vtkXMLDataElement.cxx
vtkXMLFileOutputWindow.cxx
)

# Generate data for folding Unicode strings
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkUnicodeString.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeString.cmake @ONLY)

ADD_CUSTOM_COMMAND(
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/CaseFolding.txt
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeString.cmake
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeString.cmake
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeCaseFoldData.h
  )

IF(VTK_HAVE_SYNC_BUILTINS)
  SET_SOURCE_FILES_PROPERTIES(
    vtkTimeStamp.cxx
    PROPERTIES
    COMPILE_DEFINITIONS VTK_HAVE_SYNC_BUILTINS
  )
ENDIF(VTK_HAVE_SYNC_BUILTINS)

SET_SOURCE_FILES_PROPERTIES(
  vtkUnicodeString.cxx
  OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeCaseFoldData.h
  )

IF(CMAKE_COMPILER_IS_GNUCXX)
  SET_SOURCE_FILES_PROPERTIES(
   vtkPolynomialSolversUnivariate.cxx
   PROPERTIES
   COMPILE_FLAGS -O0
   )
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

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
SET(VTK_TYPE_NATIVE_Int16 Short)
SET(VTK_TYPE_NATIVE_Int32 Int)
SET(VTK_TYPE_NATIVE_UInt8 UnsignedChar)
SET(VTK_TYPE_NATIVE_UInt16 UnsignedShort)
SET(VTK_TYPE_NATIVE_UInt32 UnsignedInt)

IF(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
  SET(VTK_TYPE_NATIVE_Int64 LongLong)
  SET(VTK_TYPE_NATIVE_UInt64 UnsignedLongLong)
ELSE(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
  IF(VTK_SIZEOF_LONG MATCHES "^8$")
    SET(VTK_TYPE_NATIVE_Int64 Long)
    SET(VTK_TYPE_NATIVE_UInt64 UnsignedLong)
  ELSE(VTK_SIZEOF_LONG MATCHES "^8$")
    IF(VTK_SIZEOF___INT64 MATCHES "^8$")
      SET(VTK_TYPE_NATIVE_Int64 __Int64)
      IF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
        SET(VTK_TYPE_NATIVE_UInt64 Unsigned__Int64)
      ENDIF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    ENDIF(VTK_SIZEOF___INT64 MATCHES "^8$")
  ENDIF(VTK_SIZEOF_LONG MATCHES "^8$")
ENDIF(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
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

# Figure out if isnan and isinf functions are present.
INCLUDE(CheckSymbolExists)
INCLUDE(CheckIncludeFile)

IF (NOT HAVE_FLOAT_H)
  CHECK_INCLUDE_FILE("float.h" HAVE_FLOAT_H)
ENDIF(NOT HAVE_FLOAT_H)

IF (HAVE_FLOAT_H)
  CHECK_SYMBOL_EXISTS(isnan "float.h" VTK_HAS_ISNAN)
  IF (NOT VTK_HAS_ISNAN)
    CHECK_SYMBOL_EXISTS(_isnan "float.h" VTK_HAS__ISNAN)
  ENDIF (NOT VTK_HAS_ISNAN)
  CHECK_SYMBOL_EXISTS(isinf "float.h" VTK_HAS_ISINF)
  IF (NOT VTK_HAS_ISINF)
    CHECK_FUNCTION_EXISTS(_isinf "float.h" VTK_HAS__ISINF)
  ENDIF (NOT VTK_HAS_ISINF)
ENDIF (HAVE_FLOAT_H)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkMathConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkMathConfigure.h
  )

# Figure out if feenableexcept and fedisableexcept functions are present
CHECK_INCLUDE_FILE("fenv.h" HAVE_FENV_H)
IF (HAVE_FENV_H)
  SET(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  IF(UNIX)
    SET(CMAKE_REQUIRED_LIBRARIES m)
  ENDIF(UNIX)
  CHECK_SYMBOL_EXISTS(feenableexcept "fenv.h" VTK_HAS_FEENABLEEXCEPT)
  SET(CMAKE_REQUIRED_LIBRARIES)
ENDIF (HAVE_FENV_H)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkFloatingPointExceptionsConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkFloatingPointExceptionsConfigure.h
  )

# header files that have constants that can be wrapped
SET(Kit_WRAP_HEADERS
  vtkSystemIncludes.h
  vtkType.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkMathConfigure.h
  ${VTK_BINARY_DIR}/vtkConfigure.h
  )

SET_SOURCE_FILES_PROPERTIES(
# vtkObjectBase is marked ABSTRACT so that instantiators are happy.
vtkAbstractArray
vtkArrayIterator
vtkArrayIteratorTemplate
vtkAbstractTransform
vtkCallbackCommand
vtkCommand
vtkCommonInformationKeyManager
vtkDataArray
vtkEventForwarderCommand
vtkFloatingPointExceptions
vtkFunctionSet
vtkGaussianRandomSequence
vtkHomogeneousTransform
vtkImplicitFunction
vtkInformationDataObjectKey
vtkInformationDoubleKey
vtkInformationDoubleVectorKey
vtkInformationIdTypeKey
vtkInformationInformationKey
vtkInformationInformationVectorKey
vtkInformationIntegerKey
vtkInformationIntegerPointerKey
vtkInformationIntegerVectorKey
vtkInformationKey
vtkInformationKeyVectorKey
vtkInformationObjectBaseKey
vtkInformationObjectBaseVectorKey
vtkInformationQuadratureSchemeDefinitionVectorKey
vtkInformationRequestKey
vtkInformationStringKey
vtkInformationStringVectorKey
vtkInformationUnsignedLongKey
vtkInitialValueProblemSolver
vtkLinearTransform
vtkObjectBase
vtkObjectFactory
vtkOldStyleCallbackCommand
vtkOverrideInformation
vtkOverrideInformationCollection
vtkParametricFunction
vtkProp
vtkRandomSequence
vtkScalarsToColors
vtkSocket
vtkStructuredData
vtkWarpTransform
vtkWindow
ABSTRACT
)

SET_SOURCE_FILES_PROPERTIES(
  vtkColor
  vtkMathUtilities
  vtkRect
  vtkVector
  vtkTuple
  HEADER_FILE_ONLY
)

SET_SOURCE_FILES_PROPERTIES(
  vtkArrayIteratorTemplate.txx
  vtkBoundingBox.cxx
  vtkBreakPoint.cxx
  vtkCallbackCommand.cxx
  vtkColor.h
  vtkCommonInformationKeyManager.cxx
  vtkDebugLeaksManager.cxx
  vtkErrorCode.cxx
  vtkEventForwarderCommand.cxx
  vtkFloatingPointExceptions
  vtkGarbageCollectorManager.cxx
  vtkIOStream.cxx
  vtkIndent.cxx
  vtkLargeInteger.cxx
  vtkMathUtilities.h
  vtkOStrStreamWrapper.cxx
  vtkOStreamWrapper.cxx
  vtkOldStyleCallbackCommand.cxx
  vtkRect.h
  vtkSmartPointerBase.cxx
  vtkStdString.cxx
  vtkTimeStamp.cxx
  vtkVariant.cxx
  vtkWeakPointerBase.cxx
  vtkUnicodeString.cxx
  vtkVector.h
  vtkTuple.h

  WRAP_EXCLUDE
)

SET_SOURCE_FILES_PROPERTIES(
  vtkColor.h
  vtkRect.h
  vtkTimeStamp.cxx
  vtkVariant.cxx
  vtkVector.h
  vtkTuple.h

  PROPERTIES WRAP_SPECIAL 1
)

# Activate code in "vtkVariant.cxx".  See the comment in the source.
SET_SOURCE_FILES_PROPERTIES(vtkVariant.cxx PROPERTIES
  COMPILE_FLAGS -DVTK_VARIANT_IMPL)


IF(VTK_USE_N_WAY_ARRAYS)

  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkArray.cxx
    vtkArrayCoordinates.cxx
    vtkArrayExtents.cxx
    vtkArrayExtentsList.cxx
    vtkArrayRange.cxx
    vtkArraySort.cxx
    vtkArrayWeights.cxx
    vtkDenseArray.h
    vtkSparseArray.h
    vtkTypedArray.h
    vtkTypeTemplate.h
    )

  SET_SOURCE_FILES_PROPERTIES(
    vtkArray.cxx
    ABSTRACT
    )

  SET_SOURCE_FILES_PROPERTIES(
    vtkArrayCoordinates.cxx
    vtkArrayExtents.cxx
    vtkArrayExtentsList.cxx
    vtkArrayRange.cxx
    vtkArraySort.cxx
    vtkArrayWeights.cxx
    vtkDenseArray.h
    vtkSparseArray.h
    vtkTypedArray.h
    vtkTypeTemplate.h
    WRAP_EXCLUDE
    )

  SET_SOURCE_FILES_PROPERTIES(
    vtkDenseArray.h
    vtkSparseArray.h
    vtkTypedArray.h
    vtkTypeTemplate.h
    HEADER_FILE_ONLY
    )

  SET_SOURCE_FILES_PROPERTIES(
    vtkArrayCoordinates.cxx
    vtkArrayExtents.cxx
    vtkArrayExtentsList.cxx
    vtkArrayRange.cxx
    vtkArraySort.cxx
    vtkArrayWeights.cxx
    vtkDenseArray.h
    vtkSparseArray.h
    vtkTypedArray.h
    vtkTypeTemplate.h
    PROPERTIES WRAP_SPECIAL 1
  )

ENDIF(VTK_USE_N_WAY_ARRAYS)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS vtkTclUtil.cxx)
SET(Kit_PYTHON_EXTRA_SRCS vtkPythonCommand.cxx)
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

IF (WIN32)
  IF (NOT BORLAND)
    SET(KIT_LIBS ${KIT_LIBS} wsock32)
  ENDIF (NOT BORLAND)
ENDIF (WIN32)

IF(NOT HAVE_SOCKETS)
  # Tell vtkSocket.cxx that there are no sockets on this platform.
  SET_SOURCE_FILES_PROPERTIES(vtkSocket.cxx PROPERTIES
    COMPILE_FLAGS -DVTK_SOCKET_FAKE_API)
ENDIF(NOT HAVE_SOCKETS)

# Need nsl to resolve gethostbyname on SunOS-5.8
# and socket also
IF(CMAKE_SYSTEM MATCHES "SunOS.*")
  SET(KIT_LIBS ${KIT_LIBS} socket nsl)
ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

IF(CMAKE_COMPILER_IS_GNUCXX AND MINGW)
  # MinGW fails to dllexport the explicit instantiations of
  # vtkDataArrayTemplate, so we must export all symbols.
  GET_TARGET_PROPERTY(VTK_COMMON_LINK_FLAGS vtkCommon LINK_FLAGS)
  IF(VTK_COMMON_LINK_FLAGS)
    SET(VTK_COMMON_LINK_FLAGS "${VTK_COMMON_LINK_FLAGS} ")
  ELSE(VTK_COMMON_LINK_FLAGS)
    SET(VTK_COMMON_LINK_FLAGS)
  ENDIF(VTK_COMMON_LINK_FLAGS)
  SET_TARGET_PROPERTIES(vtkCommon PROPERTIES
    LINK_FLAGS "${VTK_COMMON_LINK_FLAGS}-Wl,--export-all-symbols")
ENDIF(CMAKE_COMPILER_IS_GNUCXX AND MINGW)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  SET(__inst_files
    vtkABI.h
    vtkArrayIteratorTemplate.h
    vtkDataArrayTemplate.h
    vtkIOStream.h
    vtkIOStreamFwd.h
    vtkNew.h
    vtkSetGet.h
    vtkSmartPointer.h
    vtkSystemIncludes.h
    vtkTemplateAliasMacro.h
    vtkType.h
    vtkTypeTraits.h
    vtkVariantCast.h
    vtkVariantInlineOperators.h
    vtkVariantCreate.h
    vtkWeakPointer.h
    vtkWin32Header.h
    vtkWindows.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Testing/Cxx/vtkTestDriver.h
    ${CMAKE_CURRENT_BINARY_DIR}/vtkMathConfigure.h
    )

  IF(VTK_WRAP_TCL)
    SET(__inst_files ${__inst_files}
      vtkTcl.h
      vtkTclUtil.h
      vtkTk.h
      )
  ENDIF(VTK_WRAP_TCL)
  IF(VTK_WRAP_PYTHON)
    SET(__inst_files ${__inst_files}
      vtkPythonCommand.h
      )
    IF (TK_FOUND)
      IF(VTK_USE_TK)
        SET(__inst_files ${__inst_files}
          vtkTcl.h
          vtkTk.h
          )
      ENDIF(VTK_USE_TK)
    ENDIF (TK_FOUND)
  ENDIF(VTK_WRAP_PYTHON)
  IF(VTK_WRAP_JAVA)
    SET(__inst_files ${__inst_files}
      vtkJavaUtil.h
      )
  ENDIF(VTK_WRAP_JAVA)

  SET(__inst_files ${__inst_files}
    vtkArrayIteratorTemplate.txx
    vtkArrayIteratorTemplateImplicit.txx
    vtkDataArrayTemplate.txx
    vtkDataArrayTemplateImplicit.txx
    vtkDenseArray.txx
    vtkTypedArray.txx
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Cxx/vtkTestUtilities.h)

  INSTALL(FILES ${__inst_files}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development)

  SET(__inst_files
    ${VTK_SOURCE_DIR}/${KIT}/Testing/HeaderTesting.py
    ${VTK_SOURCE_DIR}/${KIT}/Testing/WindowsMangleList.py
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/FindString.tcl
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/PrintSelfCheck.tcl
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/rtImageTest.tcl
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/TestSetGet.tcl
    )
  IF(VTK_USE_PARALLEL)
    SET(__inst_files ${__inst_files}
      ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/prtImageTest.tcl)
  ENDIF(VTK_USE_PARALLEL)

  INSTALL(FILES ${__inst_files}
    DESTINATION ${VTK_INSTALL_PACKAGE_DIR_CM24}/testing
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
