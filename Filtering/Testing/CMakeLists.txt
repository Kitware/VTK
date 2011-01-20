ADD_SUBDIRECTORY(Cxx)

IF (VTK_WRAP_TCL)
  ADD_SUBDIRECTORY(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  ADD_SUBDIRECTORY(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Filtering ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Filtering"
    VTK_FILTERING_EXPORT
    vtkActor2DCollection.h
    vtkAMRBox.h
    vtkApplyFilterCommandInternal.h
    vtkArrayData.h
    vtkArrayDataAlgorithm.h
    vtkCellType.h
    vtkCompositeDataSetInternals.h
    vtkDataObjectCollection.h
    vtkDataSetCollection.h
    vtkExecutionSchedulerManager.h
    vtkFactoredArrayData.h
    vtkFactoredArrayDataAlgorithm.h
    vtkFilteringInformationKeyManager.h
    vtkGraphInternals.h
    vtkImageIterator.h
    vtkImageProgressIterator.h
    vtkMarchingCubesCases.h
    vtkMarchingSquaresCases.h
    vtkStructuredPoints.h
    )
ENDIF(PYTHON_EXECUTABLE)
