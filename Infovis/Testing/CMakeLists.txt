ADD_SUBDIRECTORY(Cxx)

IF (VTK_WRAP_TCL)
  ADD_SUBDIRECTORY(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  ADD_SUBDIRECTORY(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Infovis ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Infovis"
    VTK_INFOVIS_EXPORT
    vtkArrayNorm.h
    vtkBivariateStatisticsAlgorithmPrivate.h 
    vtkMultiCorrelativeStatisticsAssessFunctor.h 
    vtkKMeansAssessFunctor.h 
    vtkBoostGraphAdapter.h
    vtkBoostRandomSparseArraySource.h
    vtkSQLDatabaseGraphSource.h
    vtkSQLDatabaseTableSource.h
    vtkStatisticsAlgorithmPrivate.h 
    vtkUnivariateStatisticsAlgorithmPrivate.h
    )
ENDIF(PYTHON_EXECUTABLE)
