IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

SUBDIRS(Cxx)

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
