catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

source TkInteractor.tcl
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/WidgetObject.tcl


# Create a line for display of the alpha
vtkPoints points
vtkCellArray alphaLine




# Starting point.
points InsertNextPoint 0.0 0.0 4.0
points InsertNextPoint 0.0 0.4 0.0
points InsertNextPoint 0.0 10.0 0.0
points InsertNextPoint 0.0 10.2 4.0

points InsertNextPoint 4.0 0.0 4.0
points InsertNextPoint 4.0 0.4 0.0
points InsertNextPoint 4.0 10.0 0.0
points InsertNextPoint 4.0 10.2 4.0

alphaLine InsertNextCell 4
alphaLine InsertCellPoint  0
alphaLine InsertCellPoint  1
alphaLine InsertCellPoint  2
alphaLine InsertCellPoint  3

alphaLine InsertNextCell 4
alphaLine InsertCellPoint  4
alphaLine InsertCellPoint  5
alphaLine InsertCellPoint  6
alphaLine InsertCellPoint  7


vtkPolyData data
    data SetPoints points
    data SetLines alphaLine

vtkTubeFilter tube
  tube SetNumberOfSides 10
  tube SetInput data
  tube SetRadius 1.0
  tube CappingOn

vtkPolyDataMapper mapper1
  mapper1 SetInput [tube GetOutput]
vtkActor actor1 
  actor1 SetMapper mapper1
  vtkProperty bfp
  actor1 SetBackfaceProperty bfp
  [actor1 GetProperty] SetColor 1.0 0.6 0.6
  [actor1 GetBackfaceProperty] SetColor 0.0 1.0 1.0
  [actor1 GetBackfaceProperty] SetAmbient 0.5
  [actor1 GetBackfaceProperty] SetDiffuse 0.5
  [actor1 GetProperty] SetAmbient 0.5
  [actor1 GetProperty] SetDiffuse 0.5

vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetDesiredUpdateRate 20
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor1
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 400 300

# render the image
#
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4

iren Initialize
renWin SetFileName "TestTubeCap.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
