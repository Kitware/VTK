# Commented out to avoid problems with Borland dashboards
# until new tests are added to Cxx/CMakeLists.txt.
ADD_SUBDIRECTORY(Cxx)

IF (VTK_WRAP_TCL)
  ADD_SUBDIRECTORY(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  ADD_SUBDIRECTORY(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Hybrid ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Hybrid"
    VTK_HYBRID_EXPORT
    vtk3DS.h
    vtkExodusIIReaderParser.h
    vtkExodusIIReaderPrivate.h
    vtkExodusIIReaderVariableCheck.h
    vtkVRML.h
    vtkX3D.h
    vtkX3DExporterFIWriterHelper.h
    )
ENDIF(PYTHON_EXECUTABLE)
