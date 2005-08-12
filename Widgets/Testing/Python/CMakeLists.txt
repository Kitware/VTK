#
# Add tests, with the data
#
IF (VTK_PYTHON_EXE AND VTK_USE_RENDERING AND VTK_DATA_ROOT AND 
    VTK_USE_DISPLAY)
  FOREACH ( tfile
      TestImagePlaneWidget
      )
    ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
             ${VTK_SOURCE_DIR}/Widgets/Testing/Python/${tfile}.py
             -D ${VTK_DATA_ROOT}
             -B ${VTK_DATA_ROOT}/Baseline/Widgets
             )                   
  ENDFOREACH( tfile )
ENDIF (VTK_PYTHON_EXE AND VTK_USE_RENDERING AND VTK_DATA_ROOT AND 
       VTK_USE_DISPLAY)
