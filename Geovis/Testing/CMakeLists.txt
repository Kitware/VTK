#SUBDIRS(Cxx)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Geovis ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Geovis"
    VTK_GEOVIS_EXPORT
    vtkGeoPatch.h
    )
ENDIF(PYTHON_EXECUTABLE)
