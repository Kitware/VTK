vtk_module(vtkAcceleratorsPiston
  DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkIOImage #from piston
    vtkImagingHybrid #from piston
  TEST_DEPENDS
    vtkTestingRendering
    vtkInteractionWidgets
    vtkImagingSources
    vtkParallelCore
    vtkParallelMPI
    vtkRenderingParallel
  EXCLUDE_FROM_ALL
  )
