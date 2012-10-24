vtk_module(vtkCommonTransforms
  GROUPS
    StandAlone
  DEPENDS
    # Explicitely list (rather than transiently through
    # vtkCommonMath) because it allows us to turn of wrapping
    # of vtkCommonMath off but still satisfy API dependcy.
    vtkCommonCore
    vtkCommonMath
  )
