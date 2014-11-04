IF(PYTHON_EXECUTABLE AND VTK_WRAP_PYTHON)
  SET(VTK_PYTHON_FILES
    backdrop.py
    catch.py
    expr.py
    file.py
    info.py
    mccases.py
    rtImageTest.py
    )

  file(COPY ${VTK_PYTHON_FILES} DESTINATION  ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/)
ENDIF()


