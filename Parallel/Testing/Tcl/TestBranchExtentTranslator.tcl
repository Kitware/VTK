package require vtk
package require vtkinteraction

vtkImageGaussianSource gauss
  gauss SetWholeExtent 0 30 0 30 0 2
  gauss SetCenter 18 12 0
  gauss SetMaximum 1.0
  gauss SetStandardDeviation 6.0
  gauss Update

vtkBranchExtentTranslator translator
  translator SetOriginalSource [gauss GetOutput]

vtkImageClip clip1
  clip1 SetOutputWholeExtent 7 28 0 15 1 1
  clip1 SetInput [gauss GetOutput]
  [clip1 GetOutput] SetExtentTranslator translator
vtkDataSetSurfaceFilter surf1
  surf1 SetInput [clip1 GetOutput]
vtkTriangleFilter tf1
  tf1 SetInput [surf1 GetOutput]
vtkPolyDataMapper mapper1
  mapper1 SetInput [tf1 GetOutput]
  mapper1 SetScalarRange 0 1
  mapper1 SetNumberOfPieces 4
  mapper1 SetPiece 1
vtkActor actor1
  actor1 SetMapper mapper1
  actor1 SetPosition 0 0 0



# For coverage, a case where all four sides get clipped by the whole extent.
vtkImageClip clip2
  clip2 SetOutputWholeExtent 16 18 3 10 0 0
  clip2 SetInput [gauss GetOutput]
  [clip2 GetOutput] SetExtentTranslator translator
vtkDataSetSurfaceFilter surf2
  surf2 SetInput [clip2 GetOutput]
vtkTriangleFilter tf2
  tf2 SetInput [surf2 GetOutput]
vtkPolyDataMapper mapper2
  mapper2 SetInput [tf2 GetOutput]
  mapper2 SetScalarRange 0 1
  mapper2 SetNumberOfPieces 4
  mapper2 SetPiece 1
vtkActor actor2
  actor2 SetMapper mapper2
  actor2 SetPosition 15 0 0



# nothing in intersection (empty case)
vtkImageClip clip3
  clip3 SetOutputWholeExtent 1 10 0 15 0 2
  clip3 SetInput [gauss GetOutput]
  [clip3 GetOutput] SetExtentTranslator translator
vtkDataSetSurfaceFilter surf3
  surf3 SetInput [clip3 GetOutput]
vtkTriangleFilter tf3
  tf3 SetInput [surf3 GetOutput]
vtkPolyDataMapper mapper3
  mapper3 SetInput [tf3 GetOutput]
  mapper3 SetScalarRange 0 1
  mapper3 SetNumberOfPieces 4
  mapper3 SetPiece 1
vtkActor actor3
  actor3 SetMapper mapper3
  actor3 SetPosition 30 0 0









vtkRenderer ren
ren AddActor actor1
ren AddActor actor2
ren AddActor actor3

vtkRenderWindow renWin
renWin AddRenderer ren

#set cam [ren GetActiveCamera]
#ren ResetCamera



vtkRenderWindowInteractor iren
iren SetRenderWindow renWin
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}

wm withdraw .

