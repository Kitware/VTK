SUBDIRS(Cxx)

IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Filtering ${PYTHON_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
           "${VTK_SOURCE_DIR}/Filtering"
           vtkActor2DCollection.h
           vtkAMRBox.h
           vtkApplyFilterCommandInternal.h
           vtkCellType.h
           vtkDataSetCollection.h
           vtkImageIterator.h
           vtkImageProgressIterator.h
           vtkMultiBlockDataSetInternal.h
           vtkDataObjectCollection.h
           vtkHierarchicalDataSetInternal.h
           vtkHierarchicalBoxDataSetInternal.h
           vtkStructuredPoints.h
           )
ENDIF(PYTHON_EXECUTABLE)
