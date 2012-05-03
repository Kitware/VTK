if(VTK_PYTHON_EXE)
  foreach(tfile
    PythonSmoke
    TestArrayArguments
    TestExecuteMethodFinalizeCrash
    TestGhost
    TestIgnoreBTX
    TestMutable
    TestNumpySupport
    TestOperators
    TestStrings
    TestSubClass
    TestTerminationCrash
    TestVariant
    TestWeakref
    TestCommand
    )
    add_test(NAME ${vtk-module}Python-${tfile}
      COMMAND ${VTK_PYTHON_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py)
  endforeach()
endif()
