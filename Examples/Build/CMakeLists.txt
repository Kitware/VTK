IF (BUILD_TESTING)

  # vtkMy is tested using cmaketest

  GET_FILENAME_COMPONENT (CMAKE_PATH ${CMAKE_COMMAND} PATH)

  FIND_PROGRAM (CMAKETEST_COMMAND
    NAMES cmaketest
    PATH ${CMAKE_PATH}
  )

  MARK_AS_ADVANCED (
    CMAKETEST_COMMAND
  )

  IF (CMAKETEST_COMMAND)

    ADD_TEST (vtkMy
      ${CMAKETEST_COMMAND}
      ${VTK_SOURCE_DIR}/Examples/Build/vtkMy
      ${VTK_BINARY_DIR}/Examples/Build/vtkMy
      vtkmyEx1
      ${VTK_BINARY_DIR}/Examples/Build/vtkMy/bin
      vtkMy
      -DVTK_BINARY_PATH:PATH=${VTK_BINARY_DIR})

  ENDIF (CMAKETEST_COMMAND)

ENDIF (BUILD_TESTING)
