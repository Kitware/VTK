catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

#
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create pipeline
#
vtkBYUReader cow
  cow SetGeometryFileName "$VTK_DATA/Viewpoint/cow.g"

vtkTriangleFilter tris
  tris SetInput [cow GetOutput]

vtkStripper strips
  strips SetInput [tris GetOutput]
  strips Update

set ncells [[strips GetOutput] GetNumberOfCells]

vtkUnsignedCharArray cellColors
  cellColors SetNumberOfComponents 3
  cellColors SetNumberOfTuples $ncells

vtkMath rn
for { set i 0 } { $i < $ncells } { incr i } {
    cellColors InsertComponent $i 0 [rn Random 100 255]
    cellColors InsertComponent $i 1 [rn Random 100 255]
    cellColors InsertComponent $i 2 [rn Random 100 255]
}

vtkScalars cellScalars
  cellScalars SetData cellColors

[[strips GetOutput] GetCellData] SetScalars cellScalars

vtkPolyDataMapper stripMapper
    stripMapper SetInput [strips GetOutput]

vtkActor stripActor
    stripActor SetMapper stripMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor stripActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 320 240
[ren1 GetActiveCamera] Azimuth 0
[ren1 GetActiveCamera] Dolly 2.0
ren1 ResetCameraClippingRange
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
