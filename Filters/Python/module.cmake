if (VTK_WRAP_PYTHON)
  vtk_module(vtkFiltersPython
    GROUPS
      StandAlone
    DEPENDS
      vtkCommonExecutionModel
    PRIVATE_DEPENDS
      vtkWrappingPythonCore
    COMPILE_DEPENDS
      vtkPython
    OPTIONAL_PYTHON_LINK
    EXCLUDE_FROM_TCL_WRAPPING
    EXCLUDE_FROM_JAVA_WRAPPING
    TEST_DEPENDS
      vtkTestingCore
    KIT
      vtkWrapping
    )
endif ()
