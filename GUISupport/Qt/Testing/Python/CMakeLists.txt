

FOREACH(tfile
    TestTimeUtility
    TestQVTKWidget
    TestvtkQtTableView
    )
  ADD_TEST(Qt${tfile}Python ${VTK_PYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py)
ENDFOREACH(tfile)
