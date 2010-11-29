ADD_SUBDIRECTORY(Cxx)

IF (VTK_WRAP_TCL)
  ADD_SUBDIRECTORY(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  ADD_SUBDIRECTORY(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Graphics ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Graphics"
    VTK_GRAPHICS_EXPORT
    vtkButterflySubdivisionFilter.h
    vtkDijkstraGraphInternals.h
    vtkLinearSubdivisionFilter.h
    vtkLoopSubdivisionFilter.h
    vtkOutlineFilter.h
    vtkProgrammableFilter.h
    vtkProgrammableSource.h
    vtkStructuredGridOutlineFilter.h
    vtkStructuredPointsGeometryFilter.h
    vtkTableBasedClipCases.h
    )
ENDIF(PYTHON_EXECUTABLE)
