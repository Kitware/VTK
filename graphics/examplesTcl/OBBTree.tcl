catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl

vtkSTLReader reader
  reader SetFileName ../../../data/42400-IDGH.stl
vtkPolyMapper dataMapper
  dataMapper SetInput [reader GetOutput]
vtkActor model
  model SetMapper dataMapper
  [model GetProperty] SetColor 1 0 0

vtkOBBTree obb
  obb SetMaxLevel 4
  obb SetNumberOfCellsPerBucket 4
vtkSpatialRepFilter boxes
  boxes SetInput [reader GetOutput]
  boxes SetSpatialRep obb
vtkPolyMapper boxMapper
  boxMapper SetInput [boxes GetOutput]
vtkActor boxActor
  boxActor SetMapper boxMapper
  [boxActor GetProperty] SetRepresentationToWireframe

vtkRenderMaster rm
set renWin [rm MakeRenderWindow]
set ren1 [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors model
$ren1 AddActors boxActor
$ren1 SetBackground 0.1 0.2 0.4
$renWin SetSize 500 500
[$ren1 GetActiveCamera] Zoom 1.5

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize
#$renWin SetFileName OBBTree.tcl.ppm
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

