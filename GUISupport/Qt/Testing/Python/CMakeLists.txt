
set(TESTS
    TestTimeUtility
    TestQVTKWidget
    TestvtkQtTableView
    TestConnection
    )

IF(VTK_USE_QVTK_QTOPENGL)
  set(TESTS
      ${TESTS}
    TestQVTKWidget2
    )
ENDIF(VTK_USE_QVTK_QTOPENGL)

FOREACH(tfile ${TESTS})
  ADD_TEST(Qt${tfile}Python ${VTK_PYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py)
ENDFOREACH(tfile)
