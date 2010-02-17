SUBDIRS(Cxx)

#IF (VTK_WRAP_TCL)
#  SUBDIRS(Tcl)
#ENDIF (VTK_WRAP_TCL)

#IF (VTK_WRAP_PYTHON)
#  SUBDIRS(Python)
#ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Charts ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Charts"
    VTK_CHARTS_EXPORT
    vtkColor.h
    vtkDataElement.h
    vtkDataValue.h
    vtkMarkUtil.h
    vtkValueHolder.h
    vtkVector.h
    )
ENDIF(PYTHON_EXECUTABLE)
