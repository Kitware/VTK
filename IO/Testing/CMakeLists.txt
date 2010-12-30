IF (VTK_WRAP_TCL)
  ADD_SUBDIRECTORY(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  ADD_SUBDIRECTORY(Python)
ENDIF (VTK_WRAP_PYTHON)

ADD_SUBDIRECTORY(Cxx)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-IO ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/IO"
    VTK_IO_EXPORT
    vtkBase64Utilities.h
    vtkMINC.h
    vtkMySQLDatabasePrivate.h
    vtkODBCInternals.h
    vtkOffsetsManagerArray.h
    vtkPLY.h
    vtkPostgreSQLDatabasePrivate.h
    vtkXMLUtilities.h
    vtkXMLWriterC.h
    vtkXMLWriterF.h
    )
ENDIF(PYTHON_EXECUTABLE)
