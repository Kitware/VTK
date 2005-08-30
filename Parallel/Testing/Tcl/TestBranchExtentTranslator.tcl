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
  [gauss GetOutput] SetExtentTranslator translator

vtkImageClip clip1
  clip1 SetOutputWholeExtent 7 28 0 15 1 1
  clip1 SetInputConnection [gauss GetOutputPort]
vtkDataSetSurfaceFilter surf1
  surf1 SetInputConnection [clip1 GetOutputPort]
vtkTriangleFilter tf1
  tf1 SetInputConnection [surf1 GetOutputPort]
vtkPolyDataMapper mapper1
  mapper1 SetInputConnection [tf1 GetOutputPort]
  mapper1 SetScalarRange 0 1
  mapper1 SetNumberOfPieces 4
  mapper1 SetPiece 1
vtkActor actor1
  actor1 SetMapper mapper1
  actor1 SetPosition 0 0 0



# For coverage, a case where all four sides get clipped by the whole extent.
vtkImageClip clip2
  clip2 SetOutputWholeExtent 16 18 3 10 0 0
  clip2 SetInputConnection [gauss GetOutputPort]
vtkDataSetSurfaceFilter surf2
  surf2 SetInputConnection [clip2 GetOutputPort]
vtkTriangleFilter tf2
  tf2 SetInputConnection [surf2 GetOutputPort]
vtkPolyDataMapper mapper2
  mapper2 SetInputConnection [tf2 GetOutputPort]
  mapper2 SetScalarRange 0 1
  mapper2 SetNumberOfPieces 4
  mapper2 SetPiece 1
vtkActor actor2
  actor2 SetMapper mapper2
  actor2 SetPosition 15 0 0



# nothing in intersection (empty case)
vtkImageClip clip3
  clip3 SetOutputWholeExtent 1 10 0 15 0 2
  clip3 SetInputConnection [gauss GetOutputPort]
vtkDataSetSurfaceFilter surf3
  surf3 SetInputConnection [clip3 GetOutputPort]
vtkTriangleFilter tf3
  tf3 SetInputConnection [surf3 GetOutputPort]
vtkPolyDataMapper mapper3
  mapper3 SetInputConnection [tf3 GetOutputPort]
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


renWin Render

# break loop to avoid a memory leak.
translator SetOriginalSource {}
