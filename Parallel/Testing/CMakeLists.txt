SUBDIRS(Cxx)
IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Parallel ${PYTHON_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
	   "${VTK_SOURCE_DIR}/Parallel"
           vtkMPI.h
           )
ENDIF(PYTHON_EXECUTABLE)
