IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

SUBDIRS(Cxx)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-IO ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/IO"
    VTK_IO_EXPORT
    vtkPLY.h
    vtkBase64Utilities.h
    vtkXMLUtilities.h
    vtkXMLWriterC.h
    vtkXMLWriterF.h
    vtkOffsetsManagerArray.h
    )
ENDIF(PYTHON_EXECUTABLE)
