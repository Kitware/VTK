IF (VTK_USE_PARALLEL)
   SUBDIRS(ParallelProcessing)
ENDIF (VTK_USE_PARALLEL)

IF(BUILD_TESTING)
  IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.8)
    ADD_TEST(Example-vtkLocal ${CMAKE_CTEST_COMMAND}
      --build-and-test
      ${VTK_SOURCE_DIR}/Examples/Build/vtkLocal
      ${VTK_BINARY_DIR}/Examples/Build/vtkLocal
      --build-two-config
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${MAKEPROGRAM}
      --build-project VTKLOCAL
      --build-options -DVTK_DIR:PATH=${VTK_BINARY_DIR}
      --test-command
      ${VTK_BINARY_DIR}/Examples/Build/vtkLocal/bin/vtkLocalTest
      )
    ADD_TEST(Example-vtkMy ${CMAKE_CTEST_COMMAND}
      --build-and-test
      ${VTK_SOURCE_DIR}/Examples/Build/vtkMy
      ${VTK_BINARY_DIR}/Examples/Build/vtkMy
      --build-two-config
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${MAKEPROGRAM}
      --build-project VTKLOCAL
      --build-options -DVTK_DIR:PATH=${VTK_BINARY_DIR}
      --test-command
      ${VTK_BINARY_DIR}/Examples/Build/vtkMy/bin/vtkmyEx1
      )
  ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.8)
ENDIF(BUILD_TESTING)
