IF (VTK_USE_PARALLEL)
   SUBDIRS(ParallelProcessing)
ENDIF (VTK_USE_PARALLEL)

IF(BUILD_TESTING)
  IF(VTK_CMAKETEST_EXE)
    ADD_TEST(Example-vtkLocal ${VTK_CMAKETEST_EXE}
             ${VTK_SOURCE_DIR}/Examples/Build/vtkLocal
             ${VTK_BINARY_DIR}/Examples/Build/vtkLocal
             vtkLocalTest ${VTK_BINARY_DIR}/Examples/Build/vtkLocal/bin
             VTKLOCAL CMAKE_ARGS -DVTK_DIR:PATH=${VTK_BINARY_DIR})
    ADD_TEST(Example-vtkMy ${VTK_CMAKETEST_EXE}
             ${VTK_SOURCE_DIR}/Examples/Build/vtkMy
             ${VTK_BINARY_DIR}/Examples/Build/vtkMy
             vtkmyEx1 ${VTK_BINARY_DIR}/Examples/Build/vtkMy/bin
             VTKMY CMAKE_ARGS -DVTK_DIR:PATH=${VTK_BINARY_DIR})
  ENDIF(VTK_CMAKETEST_EXE)
ENDIF(BUILD_TESTING)
