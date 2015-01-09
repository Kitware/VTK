vtk_module(vtkParallelCore
  GROUPS
    StandAlone
  DEPENDS
    # Explicitely list (rather than transiently through
    # vtkIOLegacy) because it allows us to turn of wrapping
    # of vtkIOLegacy off but still satisfy API dependcy.
    vtkCommonCore
    vtkIOLegacy
  PRIVATE_DEPENDS
    vtksys
  COMPILE_DEPENDS
    vtkUtilitiesHashSource
  TEST_DEPENDS
    vtkTestingRendering
  KIT
    vtkParallel
  )
