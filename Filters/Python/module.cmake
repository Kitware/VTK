if (VTK_WRAP_PYTHON)
  vtk_module(vtkFiltersPython
    GROUPS
      StandAlone
    DEPENDS
      vtkCommonExecutionModel
      vtkPython
    PRIVATE_DEPENDS
      vtkWrappingPythonCore
    )
endif ()
