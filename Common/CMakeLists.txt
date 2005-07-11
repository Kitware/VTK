SET(KIT Common)
SET(UKIT COMMON)
SET(KIT_TCL_LIBS ${VTK_TCL_LIBRARIES})
SET(KIT_PYTHON_LIBS)
SET(KIT_JAVA_LIBS)
SET(KIT_LIBS)

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
vtkAssemblyNode.cxx
vtkAssemblyPath.cxx
vtkAssemblyPaths.cxx
vtkBitArray.cxx
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
  vtkSignedCharArray.cxx
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
    ADD_DEPENDENCIES(vtk${KIT}Java ${KIT_JAVA_DEPS})
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

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h
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
    vtkDataArrayTemplate
    vtkDataArrayTemplateImplicit
    )

  INSTALL_FILES(${VTK_INSTALL_LIB_DIR}/testing FILES
    ${VTK_SOURCE_DIR}/${KIT}/Testing/HeaderTesting.py
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/FindString.tcl
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/PrintSelfCheck.tcl
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/rtImageTest.tcl
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/TestSetGet.tcl
    )
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} FILES
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Cxx/vtkTestUtilities.h)
  IF(VTK_USE_PARALLEL)
    INSTALL_FILES(${VTK_INSTALL_LIB_DIR}/testing FILES
      ${VTK_SOURCE_DIR}/${KIT}/Testing/Tcl/prtImageTest.tcl)
  ENDIF(VTK_USE_PARALLEL)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
