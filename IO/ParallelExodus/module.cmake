vtk_module(vtkIOParallelExodus
  IMPLEMENTS
    vtkIOExodus
  DEPENDS
    vtkParallelCore
    vtkIOExodus
    vtkexodusII
  )
