catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# lines make a nice test
#vtkLineSource line1
#  line1 SetPoint1 0 0 0
#  line1 SetPoint2 1 0 0
#  line1 SetResolution 100
#vtkLineSource line2
#  line2 SetPoint1 0 0 0
#  line2 SetPoint2 1 1 1
#  line2 SetResolution 50
#vtkAppendPolyData asource
#  asource AddInput [line1 GetOutput]
#  asource AddInput [line2 GetOutput]

vtkSTLReader asource
  asource SetFileName $VTK_DATA/42400-IDGH.stl
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
  locator$i SetMaxLevel 4
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
renWin SetSize 500 200

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
vtkCamera camera
  camera SetPosition 148.579 136.352 214.961 
  camera SetFocalPoint 151.889 86.3178 223.333 
  camera SetViewAngle 30
  camera SetViewUp 0 0 -1
  camera SetClippingRange 1 100
ren1 SetActiveCamera camera
renWin Render
iren Initialize

#renWin SetFileName valid/SpatialRepAll.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

