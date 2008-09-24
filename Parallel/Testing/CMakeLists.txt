SUBDIRS(Cxx)
IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Parallel ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Parallel"
    VTK_PARALLEL_EXPORT
    vtkBlockDistribution.h
    vtkMPI.h
    vtkPBGLGraphAdapter.h
    vtkMultiProcessStream.h 
    )
ENDIF(PYTHON_EXECUTABLE)
