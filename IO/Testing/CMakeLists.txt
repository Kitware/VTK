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
    vtkMINC.h
    vtkPLY.h
    vtkBase64Utilities.h
    vtkMySQLDatabasePrivate.h
    vtkPostgreSQLDatabasePrivate.h
    vtkXMLUtilities.h
    vtkXMLWriterC.h
    vtkXMLWriterF.h
    vtkOffsetsManagerArray.h
    )
ENDIF(PYTHON_EXECUTABLE)
