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
           vtkAbstractIterator.h
           vtkAbstractList.h
           vtkAbstractMap.h
           vtkActor2DCollection.h
           vtkAMRBox.h
           vtkArrayMap.h
           vtkArrayMapIterator.h
           vtkAssemblyPaths.h
           vtkByteSwap.h
           vtkCallbackCommand.h
           vtkCellType.h
           vtkCommand.h
           vtkContainer.h
           vtkDataArrayCollection.h
           vtkDataArrayTemplate.h
           vtkDataObjectCollection.h
           vtkDataSetCollection.h
           vtkDebugLeaks.h
           vtkDebugLeaksManager.h
           vtkDynamicLoader.h
           vtkErrorCode.h
           vtkHashMap.h
           vtkHashMapIterator.h
           vtkHierarchicalDataSetInternal.h
           vtkHierarchicalBoxDataSetInternal.h
           vtkIdListCollection.h
           vtkImplicitFunctionCollection.h
           vtkIOStream.h
           vtkIOStreamFwd.h
           vtkImageIterator.h
           vtkImageProgressIterator.h
           vtkIndent.h
           vtkJavaAwt.h
           vtkJavaUtil.h
           vtkLargeInteger.h
           vtkLinkedList.h
           vtkLinkedListIterator.h
           vtkMarchingCubesCases.h
           vtkMarchingSquaresCases.h
           vtkMultiBlockDataSetInternal.h
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
           vtkStructuredPoints.h
           vtkSystemIncludes.h
           vtkTclUtil.h
           vtkTimeStamp.h
           vtkTransformCollection.h
           vtkVector.h
           vtkVectorIterator.h
           vtkWin32Header.h
           vtkWinCE.h
           vtkTcl.h
           vtkTk.h
           vtkVersion.h
           vtkXMLFileOutputWindow.h
           )
ENDIF(PYTHON_EXECUTABLE)

IF(UNIX)
  IF("x${CMAKE_INSTALL_PREFIX}" MATCHES "^x${VTK_BINARY_DIR}/InstallTest$")
    CONFIGURE_FILE(${VTK_SOURCE_DIR}/Common/Testing/Install/test.sh.in
                   ${VTK_BINARY_DIR}/Common/Testing/Install/test.sh @ONLY
                   IMMEDIATE)
    ADD_TEST(Install /bin/sh ${VTK_BINARY_DIR}/Common/Testing/Install/test.sh)
  ENDIF("x${CMAKE_INSTALL_PREFIX}" MATCHES "^x${VTK_BINARY_DIR}/InstallTest$")
ENDIF(UNIX)

# Suppress memory checking of some tests
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Common/Testing/CTestCustom.ctest.in
  ${VTK_BINARY_DIR}/Common/Testing/CTestCustom.ctest @ONLY)
