if (VTK_WRAP_PYTHON)
  vtk_module(vtkPythonContext2D
    GROUPS
      StandAlone
    COMPILE_DEPENDS
      vtkPython
    OPTIONAL_PYTHON_LINK
    EXCLUDE_FROM_JAVA_WRAPPING
    TEST_DEPENDS
      vtkTestingCore
    KIT
      vtkWrapping
    DEPENDS
      vtkRenderingContext2D
    PRIVATE_DEPENDS
      vtkCommonCore
      vtkWrappingPythonCore
    )
endif ()
