SUBDIRS(Cxx)

IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-VolumeRendering ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/VolumeRendering"
    VTK_VOLUMERENDERING_EXPORT
    vtkVolumeShearWarpDataStructure.h
    vtkVolumeProMapper.h
    )
ENDIF(PYTHON_EXECUTABLE)
