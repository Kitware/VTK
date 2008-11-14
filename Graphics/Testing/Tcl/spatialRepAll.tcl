package require vtk
package require vtkinteraction

vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkSTLReader asource
  asource SetFileName "$VTK_DATA_ROOT/Data/42400-IDGH.stl"
vtkPolyDataMapper dataMapper
  dataMapper SetInput [asource GetOutput]
vtkActor model
  model SetMapper dataMapper
  [model GetProperty] SetColor 1 0 0
  model VisibilityOn

set locators "vtkPointLocator vtkCellLocator vtkOBBTree"
set i 1
foreach locator $locators {
$locator locator$i
  locator$i AutomaticOff
  locator$i SetMaxLevel 3
vtkSpatialRepresentationFilter boxes$i
  boxes$i SetInput [asource GetOutput]
  boxes$i SetSpatialRepresentation locator$i
vtkPolyDataMapper boxMapper$i
  boxMapper$i SetInput [boxes$i GetOutput]
vtkActor boxActor$i
  boxActor$i SetMapper boxMapper$i
  boxActor$i AddPosition [expr $i * 15] 0 0
  ren1 AddActor boxActor$i
  incr i
#}


# Add the actors to the renderer, set the background and size
#
ren1 AddActor model
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 400 160

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
vtkCamera camera
  camera SetPosition 148.579 136.352 214.961 
  camera SetFocalPoint 151.889 86.3178 223.333 
  camera SetViewAngle 30
  camera SetViewUp 0 0 -1
  camera SetClippingRange 1 100
ren1 SetActiveCamera camera
renWin Render
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

