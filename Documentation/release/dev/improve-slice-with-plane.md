## Improve slice with plane filters

Slice with plane filters have the following improvements:

1) vtkPlaneCutter now delegates to vtk3DLinearGridPlaneCutter for vtkUnstructuredGrid that have only 3d linear cells.
2) vtkPlaneCutter now has the OutputPointsPrecision flag to specify the output points precision.
3) vtkPlaneCutter now has the MergePoints flag to specify if output points will be merged (default is on).
4) vtkRectilinearGrid's GetPoints has been multithreaded.
5) vtkStructuredDataPlaneCutter has been implemented that supports vtkImageData/vtkStructuredGrid/vtkRectilinearGrid and
   their subclasses and merges duplicate points.
   1) When the input is vtkImageData and GeneratePolygons is off, it delegates to vtkFlyingEdgesPlaneCutter.
6) vtkPlaneCutter now delegates to vtkStructuredDataPlaneCutter for vtkImageData/vtkStructuredGrid/vtkRectilinearGrid
   and their subclasses.
7) vtkPlaneCutter now sets the output type to vtkMultiBlockDataSet if input type is vtkUniformGridAMR.
   1) This is done to match the output of vtkAMRCutPlane.
8) vtkCutter now delegates to vtkPlaneCutter whenever possible.
