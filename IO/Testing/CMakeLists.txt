IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

SUBDIRS(Cxx)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-IO ${PYTHON_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
	   "${VTK_SOURCE_DIR}/IO"
           vtkPLY.h
           )
ENDIF(PYTHON_EXECUTABLE)
