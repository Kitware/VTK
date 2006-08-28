# Commented out to avoid problems with Borland dashboards
# until new tests are added to Cxx/CMakeLists.txt.
SUBDIRS(Cxx)

IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Hybrid ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Hybrid"
    VTK_HYBRID_EXPORT
    vtk3DS.h
    vtkVRML.h
    )
ENDIF(PYTHON_EXECUTABLE)
