SUBDIRS(Cxx)

IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Filtering ${PYTHON_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
	   "${VTK_SOURCE_DIR}/Filtering"
           )
ENDIF(PYTHON_EXECUTABLE)
