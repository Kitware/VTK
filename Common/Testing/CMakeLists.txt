SUBDIRS(Cxx)

IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Common ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Common"
    VTK_COMMON_EXPORT
    vtkAbstractIterator.h
    vtkAbstractList.h
    vtkAbstractMap.h
    vtkArrayMap.h
    vtkArrayMapIterator.h
    vtkAssemblyPaths.h
    vtkByteSwap.h
    vtkCallbackCommand.h
    vtkCommand.h
    vtkContainer.h
    vtkDataArrayCollection.h
    vtkDataArrayTemplate.h
    vtkDebugLeaks.h
    vtkDebugLeaksManager.h
    vtkDynamicLoader.h
    vtkEventForwarderCommand.h
    vtkErrorCode.h
    vtkGarbageCollector.h
    vtkGarbageCollectorManager.h
    vtkHashMap.h
    vtkHashMapIterator.h
    vtkIdListCollection.h
    vtkImplicitFunctionCollection.h
    vtkIOStream.h
    vtkIOStreamFwd.h
    vtkIndent.h
    vtkJavaAwt.h
    vtkJavaUtil.h
    vtkLargeInteger.h
    vtkLinkedList.h
    vtkLinkedListIterator.h
    vtkMultiPartExtentTranslator.h
    vtkOStrStreamWrapper.h
    vtkOStreamWrapper.h
    vtkObject.h
    vtkObjectBase.h
    vtkObjectFactoryCollection.h
    vtkOldStyleCallbackCommand.h
    vtkOverrideInformationCollection.h
    vtkPlaneCollection.h
    vtkProcessStatistics.h
    vtkPropCollection.h
    vtkPython.h
    vtkPythonUtil.h
    vtkRayCastStructures.h
    vtkRungeKutta2.h 
    vtkSetGet.h
    vtkSmartPointer.h
    vtkSmartPointerBase.h
    vtkStdString.h
    vtkStructuredData.h
    vtkSystemIncludes.h
    vtkTclUtil.h
    vtkTemplateAliasMacro.h
    vtkTimeStamp.h
    vtkTransformCollection.h
    vtkType.h
    vtkTypeTraits.h
    vtkVector.h
    vtkVectorIterator.h
    vtkWin32Header.h
    vtkWinCE.h
    vtkWindows.h
    vtkTcl.h
    vtkTk.h
    vtkVersion.h
    vtkXMLFileOutputWindow.h
    )
ENDIF(PYTHON_EXECUTABLE)

IF(UNIX)
  IF(VTK_USE_RPATH)
    SET(VTK_TEST_INSTALL 0)
  ELSE(VTK_USE_RPATH)
    STRING(COMPARE EQUAL "${CMAKE_INSTALL_PREFIX}" "${VTK_BINARY_DIR}/InstallTest"
      VTK_TEST_INSTALL)
  ENDIF(VTK_USE_RPATH)
  IF(VTK_TEST_INSTALL)
    CONFIGURE_FILE(${VTK_SOURCE_DIR}/Common/Testing/Install/test.sh.in
      ${VTK_BINARY_DIR}/Common/Testing/Install/test.sh @ONLY
      IMMEDIATE)
    ADD_TEST(Install /bin/sh ${VTK_BINARY_DIR}/Common/Testing/Install/test.sh)
  ENDIF(VTK_TEST_INSTALL)
ENDIF(UNIX)

# Suppress memory checking of some tests
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Common/Testing/CTestCustom.ctest.in
  ${VTK_BINARY_DIR}/Common/Testing/CTestCustom.ctest @ONLY)
