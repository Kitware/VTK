SUBDIRS(Cxx)

IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Imaging ${PYTHON_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
	   "${VTK_SOURCE_DIR}/Imaging"
           )
ENDIF(PYTHON_EXECUTABLE)
