package require vtktcl_interactor

vtkImageGaussianSource gauss
  gauss SetWholeExtent 0 30 0 30 0 0
  gauss SetCenter 18 12 0
  gauss SetMaximum 1.0
  gauss SetStandardDeviation 6.0
  gauss Update

vtkBranchExtentTranslator translator
  translator SetOriginalSource [gauss GetOutput]

vtkImageClip clip
  clip SetOutputWholeExtent 7 28 0 15 0 0
  clip SetInput [gauss GetOutput]
  [clip GetOutput] SetExtentTranslator translator

vtkDataSetSurfaceFilter surf
  surf SetInput [clip GetOutput]

vtkTriangleFilter tf
  tf SetInput [surf GetOutput]

vtkPolyDataMapper mapper
mapper SetInput [tf GetOutput]
mapper SetScalarRange 0 1
mapper SetNumberOfPieces 4
mapper SetPiece 1

vtkActor actor
actor SetMapper mapper
actor SetPosition 1.5 0 0

vtkRenderer ren
ren AddActor actor

vtkRenderWindow renWin
renWin AddRenderer ren

#set cam [ren GetActiveCamera]
#ren ResetCamera



vtkRenderWindowInteractor iren
iren SetRenderWindow renWin
iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}

wm withdraw .

